#pragma once

#include <algorithm>
#include <array>
#include <cassert>
#include <vector>

#include "ast/ast.hpp"
#include "func_stack.hpp"
#include "runtime/runtime.hpp"
#include "utils/util.hpp"
#include "wasm.hpp"
#include "wasm_util.hpp"

namespace wumbo
{
using namespace ast;
using namespace wasm;

struct compiler : ext_types
{
    compiler(BinaryenModuleRef mod, runtime& runtime)
        : ext_types{mod}
        , _runtime{runtime}
    {
        if (_runtime.mod == mod)
            types = _runtime.types;
    }

    runtime& _runtime;

    struct help_var_scope
    {
        function_stack& _self;
        size_t index;
        help_var_scope(function_stack& self, BinaryenType type, std::string_view name = "")
            : _self{self}
        {
            index = _self.alloc_local(type, name, true);
        }

        ~help_var_scope()
        {
            _self.free_local(index);
        }

        operator size_t()
        {
            return index;
        }
    };

    struct loop_scope
    {
        function_stack& _self;
        loop_scope(function_stack& self)
            : _self{self}
        {
            _self.push_loop();
        }

        ~loop_scope()
        {
            _self.pop_loop();
        }
    };

    struct block_scope
    {
        function_stack& _self;
        block_scope(function_stack& self)
            : _self{self}
        {
            _self.push_block();
        }

        ~block_scope()
        {
            _self.pop_block();
        }
    };

    struct function_frame
    {
        function_stack& _self;

        function_frame(function_stack& self, size_t func_arg_count, std::optional<size_t> vararg_offset = std::nullopt)
            : _self{self}
        {
            _self.push_function(func_arg_count, vararg_offset);
        }

        ~function_frame()
        {
            _self.pop_function();
        }

        std::vector<BinaryenType> get_local_type_list() const
        {
            auto func_offset = _self.functions.back().offset;

            std::vector<BinaryenType> locals;
            for (size_t i = func_offset; i < _self.vars.size(); ++i)
                locals.push_back(_self.vars[i].type);
            return locals;
        }

        void set_local_names(BinaryenFunctionRef func) const
        {
            auto func_offset = _self.functions.back().offset;

            for (size_t i = func_offset; i < _self.vars.size(); ++i)
            {
                if (!_self.vars[i].name.empty())
                {
                    auto name = _self.vars[i].name + std::to_string(i);
                    BinaryenFunctionSetLocalName(func, _self.local_offset(i), name.c_str());
                }
            }
        }

        std::vector<size_t> get_requested_upvalues()
        {
            return std::move(_self.upvalues.back());
        }
    };

    function_stack _func_stack;

    size_t function_name = 0;
    size_t label_name    = 0;

    expr_ref get_upvalue(size_t index)
    {
        auto& up = _func_stack.upvalues.back();
        int32_t result;
        if (auto iter = std::find(up.begin(), up.end(), index); iter != up.end())
        {
            result = std::distance(up.begin(), iter);
        }
        else
        {
            result = up.size();
            up.push_back(index);
        }
        auto exp = local_get(upvalue_index, upvalue_array_type());
        return array_get(exp, const_i32(result), upvalue_type());
    }

    expr_ref get_var(const name_t& name)
    {
        auto [var_type, index, type] = _func_stack.find(name);
        switch (var_type)
        {
        case var_type::local:
            if (type != upvalue_type())
                return local_get(index, anyref());
            return BinaryenStructGet(mod, 0, local_get(index, upvalue_type()), anyref(), false);
        case var_type::upvalue:
            return BinaryenStructGet(mod, 0, get_upvalue(index), anyref(), false);
        case var_type::global:
            assert(name != "_ENV" && "no environment set");
            return table_get(get_var("_ENV"), add_string(name));
        default:
            return BinaryenUnreachable(mod);
        }
    }

    expr_ref set_var(const name_t& name, expr_ref value)
    {
        auto [var_type, index, type] = _func_stack.find(name);
        switch (var_type)
        {
        case var_type::local:
            if (type != upvalue_type())
                return local_set(index, value);
            return BinaryenStructSet(mod, 0, local_get(index, upvalue_type()), value);
        case var_type::upvalue:
            return BinaryenStructSet(mod, 0, get_upvalue(index), value);
        case var_type::global:
            return table_set(get_var("_ENV"), add_string(name), value);
        default:
            return BinaryenUnreachable(mod);
        }
    }

    expr_ref _funchead(const funchead& p);
    expr_ref _functail(const functail& p, expr_ref function);

    expr_ref _vartail(const vartail& p, expr_ref var);
    expr_ref _vartail_set(const vartail& p, expr_ref var, expr_ref value);

    expr_ref _varhead(const varhead& v);

    expr_ref _varhead_set(const varhead& v, expr_ref value);
    expr_ref at_or_null(size_t array, size_t index, expr_ref value = nullptr)
    {
        return BinaryenIf(mod,
                          BinaryenRefIsNull(mod, value ? local_tee(array, value, ref_array_type()) : local_get(array, ref_array_type())),
                          null(),
                          BinaryenIf(mod,
                                     BinaryenBinary(mod, BinaryenGtUInt32(), BinaryenArrayLen(mod, local_get(array, ref_array_type())), const_i32(index)),
                                     BinaryenArrayGet(mod, local_get(array, ref_array_type()), const_i32(index), anyref(), false),
                                     null()));
    }

    expr_ref_list operator()(const function_call& p);

    expr_ref_list operator()(const assignments& p);

    auto operator()(const label_statement& p)
    {
        expr_ref_list result;

        auto& label_stack         = _func_stack.functions.back().label_stack;
        auto& request_label_stack = _func_stack.functions.back().request_label_stack;

        if (auto iter = std::find(request_label_stack.begin(), request_label_stack.end(), p.name); iter != request_label_stack.end())
        {
            BinaryenBreak(mod, p.name.c_str(), nullptr, nullptr);
        }

        //auto rl = RelooperCreate(mod);
        //RelooperAddBlock(rl );
        //RelooperRenderAndDispose(rl, )

        label_stack.push_back(p.name);
        return result;
    }
    auto operator()(const goto_statement& p)
    {
        expr_ref_list result;

        auto& label_stack         = _func_stack.functions.back().label_stack;
        auto& request_label_stack = _func_stack.functions.back().request_label_stack;
        if (auto iter = std::find(label_stack.begin(), label_stack.end(), p.name); iter != label_stack.end())
        {
            BinaryenBreak(mod, p.name.c_str(), nullptr, nullptr);
        }
        else
            request_label_stack.push_back(p.name);

        return result;
    }

    expr_ref_list operator()(const do_statement& p)
    {
        return (*this)(p.inner);
    }

    expr_ref_list operator()(const if_statement& p)
    {
        expr_ref last  = nullptr;
        expr_ref first = nullptr;
        for (auto& [cond_exp, body] : p.cond_block)
        {
            auto cond = (*this)(cond_exp);
            auto next = make_if(_runtime.call(functions::to_bool, cond), make_block((*this)(body)));
            if (last)
                BinaryenIfSetIfFalse(last, next);
            else
                first = next;
            last = next;
        }
        if (p.else_block)
            BinaryenIfSetIfFalse(last, make_block((*this)(*p.else_block)));

        return {first};
    }

    expr_ref_list operator()(const for_statement& p);
    expr_ref_list operator()(const key_break& p);
    expr_ref_list operator()(const while_statement& p);
    expr_ref_list operator()(const repeat_statement& p);
    expr_ref_list operator()(const for_each& p);

    expr_ref_list operator()(const function_definition& p);
    expr_ref_list operator()(const local_function& p);
    expr_ref_list operator()(const local_variables& p);

    expr_ref operator()(const expression& p);

    expr_ref make_ref_array(const expr_ref_list& p);
    expr_ref make_ref_array(expr_ref p);
    expr_ref operator()(const expression_list& p);

    auto operator()(const nil&)
    {
        return null();
    }

    auto operator()(const boolean& p)
    {
        return new_boolean(const_boolean(p.value));
    }

    auto operator()(const int_type& p)
    {
        return new_integer(const_integer(p));
    }

    auto operator()(const float_type& p)
    {
        return new_number(const_number(p));
    }

    expr_ref operator()(const literal& p)
    {
        return add_string(p.str);
    }

    expr_ref operator()(const ellipsis& p)
    {
        auto vararg_offset = _func_stack.functions.back().vararg_offset;
        if (vararg_offset)
        {
            return local_get(*vararg_offset, ref_array_type());
        }
        semantic_error("cannot use '...' outside a vararg function near '...'");
    }

    expr_ref_list unpack_locals(const name_list& p, expr_ref list, bool is_vararg = false)
    {
        auto offset                    = _func_stack.local_offset();
        std::string none               = "+none" + std::to_string(label_name++);
        std::vector<const char*> names = {none.c_str()};
        expr_ref_list result;

        if (p.empty())
        {
            if (is_vararg)
                _func_stack.functions.back().vararg_offset = args_index;
            return result;
        }

        for (auto& arg : p)
        {
            _func_stack.alloc_lua_local(arg, anyref());
            names.push_back(arg.c_str());
        }

        const char* vararg = "...";
        if (is_vararg)
        {
            names.push_back(vararg);
        }

        std::array<expr_ref, 2> exp = {
            BinaryenSwitch(mod,
                           std::data(names),
                           std::size(names) - 1,
                           names.back(),
                           array_len(BinaryenBrOn(mod,
                                                  BinaryenBrOnNull(),
                                                  names[0],
                                                  list,
                                                  BinaryenTypeNone())),
                           nullptr),

        };
        if (is_vararg)
        {
            exp[0] = BinaryenBlock(mod, vararg, std::data(exp), 1, BinaryenTypeAuto());

            auto new_array                             = _func_stack.alloc_lua_local(vararg, ref_array_type());
            _func_stack.functions.back().vararg_offset = new_array;

            auto size = help_var_scope{_func_stack, size_type()};

            auto calc_size = local_tee(size, BinaryenBinary(mod, BinaryenSubInt32(), array_len(list), const_i32(p.size())), size_type());
            exp[1]         = BinaryenArrayCopy(mod,
                                       local_tee(new_array,
                                                 BinaryenArrayNew(mod,
                                                                  BinaryenTypeGetHeapType(ref_array_type()),
                                                                  calc_size,
                                                                  nullptr),
                                                 ref_array_type()),
                                       const_i32(0),
                                       list,
                                       const_i32(p.size()),
                                       local_get(size, size_type()));
        }

        bool first = !is_vararg;
        size_t j   = p.size();
        for (auto iter = p.rbegin(); iter != p.rend(); ++iter)
        {
            j--;
            exp[0] = BinaryenBlock(mod, iter->c_str(), std::data(exp), first ? 1 : std::size(exp), BinaryenTypeAuto());
            first  = false;

            auto get = array_get(
                list,
                const_i32(j),
                anyref());

            exp[1] = local_set(j + offset, get);
        }

        result.push_back(make_block(exp, names[0]));
        return result;
    }

    template<typename F>
    auto add_func(const char* name, const name_list& p, bool vararg, F&& f)
    {
        function_frame frame{_func_stack, func_arg_count};

        expr_ref_list body = unpack_locals(p, local_get(args_index, ref_array_type()), vararg);

        append(body, f());

        body.push_back(make_return(null()));

        auto locals = frame.get_local_type_list();

        auto result = BinaryenAddFunctionWithHeapType(mod,
                                                      name,
                                                      BinaryenTypeGetHeapType(lua_func()),
                                                      std::data(locals),
                                                      std::size(locals),
                                                      make_block(body));

        BinaryenFunctionSetLocalName(result, args_index, "args");
        BinaryenFunctionSetLocalName(result, upvalue_index, "upvalues");

        frame.set_local_names(result);

        return std::tuple{result, frame.get_requested_upvalues()};
    }

    expr_ref_list gather_upvalues(const std::vector<size_t>& req_ups)
    {
        expr_ref_list ups;

        for (auto index : req_ups)
        {
            if (_func_stack.is_index_local(index)) // local
            {
                auto& var        = _func_stack.vars[index];
                auto local_index = _func_stack.local_offset(index);
                if (var.type != upvalue_type())
                {
                    expr_ref val = local_get(local_index, anyref());
                    local_index  = _func_stack.alloc_lua_local(var.current_name(), upvalue_type());
                    ups.push_back(make_block(std::array{
                        BinaryenStructSet(mod, 0, local_tee(local_index, BinaryenStructNew(mod, nullptr, 0, BinaryenTypeGetHeapType(upvalue_type())), upvalue_type()), val),
                        local_get(local_index, upvalue_type()),
                    }));
                }
                else
                    ups.push_back(local_get(local_index, upvalue_type()));
            }
            else // upvalue
            {
                ups.push_back(get_upvalue(index));
            }
        }
        return ups;
    }

    template<typename F>
    auto add_func_ref(const char* name, const name_list& p, bool vararg, F&& f)
    {
        auto [func, req_ups] = add_func(name, p, vararg, f);

        auto ups = gather_upvalues(req_ups);

        auto sig       = BinaryenTypeFromHeapType(BinaryenFunctionGetType(func), false);
        expr_ref exp[] = {
            BinaryenRefFunc(mod, name, sig),
            ups.empty() ? null() : BinaryenArrayNewFixed(mod, BinaryenTypeGetHeapType(upvalue_array_type()), std::data(ups), std::size(ups)),
        };
        return BinaryenStructNew(mod, std::data(exp), std::size(exp), BinaryenTypeGetHeapType(type<value_type::function>()));
    }

    auto add_func_ref(const char* name, const block& inner, const name_list& p, bool vararg)
    {
        return add_func_ref(
            name, p, vararg, [&]()
            {
                return (*this)(inner);
            });
    }

    auto add_func_ref(const char* name, const function_body& p) -> expr_ref
    {
        return add_func_ref(name, p.inner, p.params, p.vararg);
    }

    auto operator()(const function_body& p)
    {
        return add_func_ref(std::to_string(function_name++).c_str(), p);
    }

    expr_ref operator()(const prefixexp& p);

    template<typename T>
    auto operator()(const box<T>& p)
    {
        return (*this)(*p);
    }

    expr_ref calc_hash(expr_ref key);
    expr_ref find_bucket(expr_ref table, expr_ref hash);

    expr_ref table_get(expr_ref table, expr_ref key);

    expr_ref table_set(expr_ref table, expr_ref key, expr_ref value);

    expr_ref operator()(const table_constructor& p);

    expr_ref call(expr_ref func, expr_ref args);

    expr_ref_list open_basic_lib();
    expr_ref_list setup_env();

    expr_ref operator()(const bin_operation& p);

    expr_ref operator()(const un_operation& p);

    auto operator()(const block& p) -> expr_ref_list
    {
        block_scope block{_func_stack};

        expr_ref_list result;
        for (auto& statement : p.statements)
        {
            auto temp = std::visit(*this, statement.inner);
            result.insert(result.end(), temp.begin(), temp.end());
        }
        if (p.retstat)
        {
            auto list = (*this)(*p.retstat);
            result.push_back(make_return(list));
        }

        return result;
    }

    auto convert(const block& chunk)
    {
        function_frame frame{_func_stack, 0, std::nullopt};

        auto env = setup_env();

        append(env, open_basic_lib());

        auto start = add_func_ref("*init", chunk, {}, true);

        env.push_back(BinaryenDrop(mod, call(start, null())));

        const char* tags[] = {error_tag};
        expr_ref catches[] = {
            make_block(std::array{
                BinaryenDrop(mod, BinaryenPop(mod, anyref())),
                BinaryenNop(mod),
                //BinaryenUnreachable(mod),
            })};

        auto try_ = BinaryenTry(mod,
                                nullptr,
                                make_block(env),
                                std::data(tags),
                                std::size(tags),
                                std::data(catches),
                                std::size(catches),
                                nullptr);

        auto locals = frame.get_local_type_list();
        BinaryenAddFunction(mod,
                            "*invoke_lua",
                            BinaryenTypeNone(),
                            BinaryenTypeNone(),
                            std::data(locals),
                            std::size(locals),
                            try_);

        BinaryenAddFunctionExport(mod, "*invoke_lua", "start");
    }
};
} // namespace wumbo
