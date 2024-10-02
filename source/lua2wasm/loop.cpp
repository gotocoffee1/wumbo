#include "compiler.hpp"

namespace wumbo
{

static std::string loop_end(compiler* self)
{
    return "loop_end" + std::to_string(self->_func_stack.loop_counter);
}

static std::string loop_begin(compiler* self)
{
    return "loop_begin" + std::to_string(self->_func_stack.loop_counter);
}

std::vector<BinaryenExpressionRef> compiler::operator()(const key_break& p)
{
    if (_func_stack.loop_stack.back() > 0)
    {
        auto end = loop_end(this);

        return {BinaryenBreak(mod, end.c_str(), nullptr, nullptr)};
    }

    return {throw_error(null())};
}
std::vector<BinaryenExpressionRef> compiler::operator()(const while_statement& p)

{
    loop_scope scope{_func_stack};
    auto cond = (*this)(p.condition);

    auto body = (*this)(p.inner);

    auto begin = loop_begin(this);
    auto end   = loop_end(this);

    body.push_back(BinaryenBreak(mod, begin.c_str(), make_call("*to_bool", cond, size_type()), nullptr));

    return {make_block(std::array{
                           BinaryenBreak(mod, end.c_str(), BinaryenUnary(mod, BinaryenEqZInt32(), make_call("*to_bool", cond, size_type())), nullptr),
                           BinaryenLoop(mod, begin.c_str(), make_block(body)),
                       },
                       end.c_str())};
}
std::vector<BinaryenExpressionRef> compiler::operator()(const repeat_statement& p)
{
    loop_scope scope{_func_stack};

    auto cond = (*this)(p.condition);

    auto begin = loop_begin(this);
    auto end   = loop_end(this);

    auto body = (*this)(p.inner);
    body.push_back(BinaryenBreak(mod, begin.c_str(), BinaryenUnary(mod, BinaryenEqZInt32(), make_call("*to_bool", cond, size_type())), nullptr));
    return {BinaryenLoop(mod, begin.c_str(), make_block(body, end.c_str()))};
}

std::vector<BinaryenExpressionRef> compiler::operator()(const for_statement& p)
{
    loop_scope scope{_func_stack};
    auto begin = loop_begin(this);
    auto end   = loop_end(this);
    std::vector<BinaryenExpressionRef> result;
    return result;
}
std::vector<BinaryenExpressionRef> compiler::operator()(const for_each& p)
{
    loop_scope scope{_func_stack};
    auto begin = loop_begin(this);
    auto end   = loop_end(this);
    std::vector<BinaryenExpressionRef> result;
    return result;
}
} // namespace wumbo
