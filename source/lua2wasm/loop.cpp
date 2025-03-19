#include "compiler.hpp"

namespace wumbo
{

static std::string loop_end(compiler* self)
{
    return "*loop_end" + std::to_string(self->_func_stack.loop_counter);
}

static std::string loop_begin(compiler* self)
{
    return "*loop_begin" + std::to_string(self->_func_stack.loop_counter);
}

expr_ref_list compiler::operator()(const key_break& p)
{
    if (_func_stack.loop_stack.back() > 0)
    {
        auto end = loop_end(this);

        return {BinaryenBreak(mod, end.c_str(), nullptr, nullptr)};
    }

    semantic_error("break outside loop");
}

expr_ref_list compiler::operator()(const while_statement& p)

{
    loop_scope scope{_func_stack};
    auto cond = (*this)(p.condition);

    auto body = (*this)(p.inner);

    auto begin = loop_begin(this);
    auto end   = loop_end(this);

    body.push_back(BinaryenBreak(mod, begin.c_str(), _runtime.call(functions::to_bool, cond), nullptr));

    return {make_block(std::array{
                           BinaryenBreak(mod, end.c_str(), _runtime.call(functions::to_bool_not, cond), nullptr),
                           BinaryenLoop(mod, begin.c_str(), make_block(body)),
                       },
                       end.c_str())};
}
expr_ref_list compiler::operator()(const repeat_statement& p)
{
    loop_scope scope{_func_stack};

    auto cond = (*this)(p.condition);

    auto begin = loop_begin(this);
    auto end   = loop_end(this);

    auto body = (*this)(p.inner);
    body.push_back(BinaryenBreak(mod, begin.c_str(), _runtime.call(functions::to_bool_not, cond), nullptr));
    return {BinaryenLoop(mod, begin.c_str(), make_block(body, end.c_str()))};
}

expr_ref_list compiler::operator()(const for_statement& p)
{
    loop_scope scope{_func_stack};
    auto begin = loop_begin(this);
    auto end   = loop_end(this);
    expr_ref_list result;
    return result;
}
expr_ref_list compiler::operator()(const for_each& p)
{
    // https://www.lua.org/manual/5.3/manual.html#3.3.5

    block_scope block{_func_stack};

    local_variables vars_help;
    vars_help.explist = p.explist;
    vars_help.names   = {"*f", "*s", "*var"};

    expr_ref_list result = {(*this)(vars_help)};

    loop_scope scope{_func_stack};

    local_variables vars;
    vars.names = p.names;

    auto& call = vars.explist.emplace_back().inner.emplace<box<prefixexp>>();
    call->chead.emplace<name_t>("*f");
    auto& args = call->tail.emplace_back().emplace<functail>().args;
    args.emplace_back().inner.emplace<box<prefixexp>>()->chead.emplace<name_t>("*s");
    args.emplace_back().inner.emplace<box<prefixexp>>()->chead.emplace<name_t>("*var");

    auto begin = loop_begin(this);
    auto end   = loop_end(this);

    expr_ref_list inner = (*this)(vars);

    inner.push_back(BinaryenBreak(mod,
                                  end.c_str(),
                                  BinaryenRefIsNull(mod, get_var(p.names[0])),
                                  nullptr));

    inner.push_back(set_var("*var", get_var(p.names[0])));

    append(inner, (*this)(p.inner));

    inner.push_back(BinaryenBreak(mod, begin.c_str(), nullptr, nullptr));

    result.push_back(BinaryenLoop(mod, begin.c_str(), make_block(inner, end.c_str())));

    return result;
}
} // namespace wumbo
