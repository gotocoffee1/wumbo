#pragma once

#include <algorithm>
#include <array>
#include <cassert>
#include <nonstd/span.hpp>
#include <utility>
#include <vector>

#include "ast/ast.hpp"
#include "binaryen-c.h"
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
        auto exp = local_get(upvalue_index, ref_array_type());
        return array_get(exp, const_i32(result), anyref());
        //return array_get(exp, const_i32(result), upvalue_type());
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
            if (type != upvalue_type())
                return get_upvalue(index);
            return BinaryenStructGet(mod, 0, BinaryenRefCast(mod, get_upvalue(index), upvalue_type()), anyref(), false);
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
            assert(type == upvalue_type() && "must be upvalue");
            return BinaryenStructSet(mod, 0, BinaryenRefCast(mod, get_upvalue(index), upvalue_type()), value);
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

    template<typename F>
    expr_ref at_or_null(size_t array, size_t index, expr_ref value, F&& f)
    {
        return make_if(BinaryenRefIsNull(mod, value ? local_tee(array, value, ref_array_type()) : local_get(array, ref_array_type())),
                       f(),
                       BinaryenIf(mod,
                                  BinaryenBinary(mod, BinaryenGtUInt32(), BinaryenArrayLen(mod, local_get(array, ref_array_type())), const_i32(index)),
                                  BinaryenArrayGet(mod, local_get(array, ref_array_type()), const_i32(index), anyref(), false),
                                  f()));
    }

    expr_ref at_or_null(size_t array, size_t index, expr_ref value = nullptr)
    {
        return at_or_null(array, index, value, [&]()
                          {
                              return null();
                          });
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

    expr_ref_list unpack_locals(const name_list& p, expr_ref list, nonstd::span<const local_usage> usage, bool is_vararg = false)
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
        size_t i = 0;
        for (auto& arg : p)
        {
            names.push_back(arg.c_str());

            //local_tee(local_index, BinaryenStructNew(mod, &val, 1, BinaryenTypeGetHeapType(upvalue_type())), upvalue_type()));

            if (usage[i++].is_upvalue())
            {
                auto index = _func_stack.alloc_lua_local(arg, upvalue_type());

                result.push_back(local_set(index,
                                           BinaryenStructNew(
                                               mod,
                                               nullptr,
                                               0,
                                               BinaryenTypeGetHeapType(upvalue_type()))));
            }
            else
                _func_stack.alloc_lua_local(arg, anyref());
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

            exp[1] = local_set(j + offset,
                               usage[j].is_upvalue() ? BinaryenStructNew(
                                                           mod,
                                                           &get,
                                                           1,
                                                           BinaryenTypeGetHeapType(upvalue_type()))
                                                     : get);
        }

        result.push_back(make_block(exp, names[0]));
        return result;
    }

    template<typename F>
    auto add_func(const char* name, const name_list& p, nonstd::span<const local_usage> usage, bool vararg, F&& f)
    {
        function_frame frame{_func_stack, func_arg_count};

        expr_ref_list body = unpack_locals(p, local_get(args_index, ref_array_type()), usage, vararg);

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
                auto local_index = _func_stack.local_offset(index);
                ups.push_back(local_get(local_index, _func_stack.vars[index].type));
            }
            else // upvalue
            {
                ups.push_back(get_upvalue(index));
            }
        }
        return ups;
    }

    template<typename F>
    auto get_func_ref(const char* name, const name_list& p, nonstd::span<const local_usage> usage, bool vararg, F&& f)
    {
        auto [func, req_ups] = add_func(name, p, usage, vararg, f);

        auto ups = gather_upvalues(req_ups);

        auto sig = BinaryenTypeFromHeapType(BinaryenFunctionGetType(func), false);
        return std::tuple{BinaryenRefFunc(mod, name, sig), std::move(ups)};
    }

    template<typename F>
    auto add_func_ref(const char* name, const name_list& p, nonstd::span<const local_usage> usage, bool vararg, F&& f)
    {
        auto [ref, ups] = get_func_ref(name, p, usage, vararg, std::forward<F>(f));
        return build_closure(ref, std::move(ups));
    }

    auto add_func_ref(const char* name, const block& inner, const name_list& p, nonstd::span<const local_usage> usage, bool vararg)
    {
        return add_func_ref(
            name, p, usage, vararg, [&]()
            {
                return (*this)(inner);
            });
    }

    auto add_func_ref(const char* name, const function_body& p) -> expr_ref
    {
        return add_func_ref(name, p.inner, p.params, p.usage, p.vararg);
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

    expr_ref_list setup_env()
    {
        local_variables vars;
        vars.names.push_back("_ENV");
        vars.explist.emplace_back().inner = table_constructor{};
        auto& usage                       = vars.usage.emplace_back();
        usage.upvalue                     = true;
        usage.read_count                  = 1;
        usage.write_count                 = 0;

        return (*this)(vars);

        // todo:
        //local _ENV = {}
        //_ENV._G = _ENV
    }

    auto convert(const block& chunk)
    {
        function_frame frame{_func_stack, 1, std::nullopt};

        auto env = setup_env();

        append(env, std::array{
                        drop(_runtime.call(functions::open_basic_lib, std::array{BinaryenRefCast(mod, get_var("_ENV"), type<value_type::table>())})),
                        set_var("coroutine", _runtime.call(functions::open_coroutine_lib, std::array{_runtime.call(functions::table_create_map, std::array{const_i32(7)})})),
                        set_var("table", _runtime.call(functions::open_table_lib, std::array{_runtime.call(functions::table_create_map, std::array{const_i32(7)})})),
                        set_var("io", _runtime.call(functions::open_io_lib, std::array{_runtime.call(functions::table_create_map, std::array{const_i32(11)})})),
                        set_var("os", _runtime.call(functions::open_os_lib, std::array{_runtime.call(functions::table_create_map, std::array{const_i32(11)})})),
                        set_var("package", _runtime.call(functions::open_package_lib, std::array{_runtime.call(functions::table_create_map, std::array{const_i32(8)})})),
                        set_var("string", _runtime.call(functions::open_string_lib, std::array{_runtime.call(functions::table_create_map, std::array{const_i32(17)})})),
                        set_var("math", _runtime.call(functions::open_math_lib, std::array{_runtime.call(functions::table_create_map, std::array{const_i32(27)})})),
                        set_var("utf8", _runtime.call(functions::open_utf8_lib, std::array{_runtime.call(functions::table_create_map, std::array{const_i32(6)})})),
                        set_var("debug", _runtime.call(functions::open_debug_lib, std::array{_runtime.call(functions::table_create_map, std::array{const_i32(17)})})),
                    });

        auto init = add_func_ref("*init", chunk, {}, {}, true);
        BinaryenAddFunctionExport(mod, "*init", "init");
        // call init function with ... args
        env.push_back(call(init, local_get(0, ref_array_type())));

        export_func(_runtime.require(functions::get_type_num).name);
        export_func(_runtime.require(functions::to_js_integer).name);
        export_func(_runtime.require(functions::to_js_string).name);
        export_func(_runtime.require(functions::any_array_get).name);
        export_func(_runtime.require(functions::any_array_set).name);
        export_func(_runtime.require(functions::any_array_create).name);
        export_func(_runtime.require(functions::any_array_size).name);
        export_func(_runtime.require(functions::box_number).name);
        export_func(_runtime.require(functions::box_integer).name);

        auto locals = frame.get_local_type_list();
        BinaryenAddFunction(mod,
                            "*init_env",
                            ref_array_type(),
                            ref_array_type(),
                            std::data(locals),
                            std::size(locals),
                            make_block(env));

        BinaryenAddFunctionExport(mod, "*init_env", "init_env");
    }
};
} // namespace wumbo
