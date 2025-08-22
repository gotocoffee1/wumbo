#pragma once

#include "backend/func_stack.hpp"
#include "backend/wasm_util.hpp"
#include "binaryen-c.h"
#include <cassert>
#include <type_traits>

#define RUNTIME_FUNCTIONS(DO)                                                                   \
    DO(table_get, create_type(anyref(), anyref()), anyref())                                    \
    DO(table_set, create_type(anyref(), anyref(), anyref()), BinaryenTypeNone())                \
    DO(table_create_array, ref_array_type(), get_type<table>())                                 \
    DO(table_create_map, size_type(), get_type<table>())                                        \
    DO(to_bool, anyref(), bool_type())                                                          \
    DO(to_bool_not, anyref(), bool_type())                                                      \
    DO(logic_not, anyref(), anyref())                                                           \
    DO(binary_not, anyref(), anyref())                                                          \
    DO(minus, anyref(), anyref())                                                               \
    DO(len, anyref(), anyref())                                                                 \
    DO(addition, create_type(anyref(), anyref()), anyref())                                     \
    DO(subtraction, create_type(anyref(), anyref()), anyref())                                  \
    DO(multiplication, create_type(anyref(), anyref()), anyref())                               \
    DO(division, create_type(anyref(), anyref()), anyref())                                     \
    DO(division_floor, create_type(anyref(), anyref()), anyref())                               \
    DO(exponentiation, create_type(anyref(), anyref()), anyref())                               \
    DO(modulo, create_type(anyref(), anyref()), anyref())                                       \
    DO(binary_or, create_type(anyref(), anyref()), anyref())                                    \
    DO(binary_and, create_type(anyref(), anyref()), anyref())                                   \
    DO(binary_xor, create_type(anyref(), anyref()), anyref())                                   \
    DO(binary_right_shift, create_type(anyref(), anyref()), anyref())                           \
    DO(binary_left_shift, create_type(anyref(), anyref()), anyref())                            \
    DO(equality, create_type(anyref(), anyref()), anyref())                                     \
    DO(inequality, create_type(anyref(), anyref()), anyref())                                   \
    DO(less_than, create_type(anyref(), anyref()), anyref())                                    \
    DO(greater_than, create_type(anyref(), anyref()), anyref())                                 \
    DO(less_or_equal, create_type(anyref(), anyref()), anyref())                                \
    DO(greater_or_equal, create_type(anyref(), anyref()), anyref())                             \
    DO(to_string, anyref(), type<value_type::string>())                                         \
    DO(to_number, anyref(), anyref())                                                           \
    DO(lua_str_to_js_array, type<value_type::string>(), BinaryenTypeExternref())                \
    DO(js_array_to_lua_str, BinaryenTypeExternref(), type<value_type::string>())                \
    DO(get_type_num, anyref(), size_type())                                                     \
    DO(box_integer, integer_type(), type<value_type::integer>())                                \
    DO(box_number, number_type(), type<value_type::number>())                                   \
    DO(to_js_integer, anyref(), integer_type())                                                 \
    DO(to_js_string, anyref(), BinaryenTypeExternref())                                         \
    DO(any_array_size, ref_array_type(), size_type())                                           \
    DO(any_array_create, size_type(), ref_array_type())                                         \
    DO(any_array_get, create_type(ref_array_type(), size_type()), anyref())                     \
    DO(any_array_set, create_type(ref_array_type(), size_type(), anyref()), BinaryenTypeNone()) \
    DO(open_basic_lib, get_type<table>(), get_type<table>())                                    \
    DO(open_coroutine_lib, get_type<table>(), get_type<table>())                                \
    DO(open_table_lib, get_type<table>(), get_type<table>())                                    \
    DO(open_io_lib, get_type<table>(), get_type<table>())                                       \
    DO(open_os_lib, get_type<table>(), get_type<table>())                                       \
    DO(open_string_lib, get_type<table>(), get_type<table>())                                   \
    DO(open_math_lib, get_type<table>(), get_type<table>())                                     \
    DO(open_utf8_lib, get_type<table>(), get_type<table>())                                     \
    DO(open_debug_lib, get_type<table>(), get_type<table>())                                    \
    DO(invoke, create_type(anyref(), ref_array_type()), ref_array_type())

namespace wumbo
{
#define ENUM_LIST(name, ...) name,
enum class functions
{
    RUNTIME_FUNCTIONS(ENUM_LIST)
        count,
}; // namespace functions
#undef ENUM_LIST

enum class function_action
{
    none,
    required,
    all,
};

using build_return_t = std::tuple<std::vector<BinaryenType>, expr_ref>;

struct runtime;
using build_func_t = build_return_t (runtime::*)();

struct func_sig
{
    const char* name;
    BinaryenType params_type;
    BinaryenType return_type;
    build_func_t build;
};

struct runtime : ext_types
{
    function_action import_functions = function_action::none;
    function_action create_functions = function_action::none;
    function_action export_functions = function_action::none;
    std::vector<bool> _required_functions;
    std::array<func_sig, static_cast<size_t>(functions::count)> _funcs;

    void build_types();
    void build();

    struct op;
    struct tbl;

#define DECL_FUNCS(name, ...) build_return_t name();
    RUNTIME_FUNCTIONS(DECL_FUNCS)
#undef DECL_FUNCS

    struct function_stack
    {
        struct var
        {
            std::string name;
            size_t name_offset;
            using flag_t = uint8_t;
            flag_t flags = 0;

            const char* current_name() const
            {
                return name.c_str() + (name.size() - name_offset);
            }
            static constexpr flag_t is_used = 1 << 0;
        };
        BinaryenModuleRef mod;

        std::vector<var> vars;
        std::vector<BinaryenType> types;
        size_t var_index = 0;

        size_t alloc(BinaryenType type, std::string_view name = "")
        {
            for (size_t i = 0; i < vars.size(); ++i)
            {
                auto& var = vars[i];
                if (type == types[i] && !(var.flags & var::is_used))
                {
                    var.name += name;
                    var.name_offset = name.size();
                    var.flags |= var::is_used;
                    return i;
                }
            }
            auto& var       = vars.emplace_back();
            var.name        = name;
            var.name_offset = name.size();
            var.flags       = var::is_used;
            types.emplace_back(type);

            return vars.size() - 1;
        }

        void locals()
        {
            var_index = vars.size();
        }

        void free_local(size_t pos)
        {
            assert(pos < vars.size() && "invalid local index");
            vars[pos].flags &= ~var::is_used;
        }

        struct func_t
        {
            BinaryenModuleRef mod;
            std::string name;
            BinaryenType ret_type;

            expr_ref operator()(nonstd::span<const expr_ref> param, bool return_call = false)
            {
                if (return_call)
                    return BinaryenReturnCall(mod, name.c_str(), const_cast<expr_ref*>(param.data()), param.size(), ret_type);
                return BinaryenCall(mod, name.c_str(), const_cast<expr_ref*>(param.data()), param.size(), ret_type);
            }

            expr_ref get_ref()
            {
                auto sig = BinaryenTypeFromHeapType(BinaryenFunctionGetType(BinaryenGetFunction(mod, name.c_str())), false);
                return BinaryenRefFunc(mod, name.c_str(), sig);
            }
        };

        template<typename F>
        func_t add_function(const std::string& name, BinaryenType ret_type, F&& body)
        {
            if (!BinaryenGetFunction(mod, name.c_str()))
            {
                auto b    = body(*this);
                auto func = BinaryenAddFunction(mod,
                                                name.c_str(),
                                                BinaryenTypeCreate(std::data(types), var_index),
                                                ret_type,
                                                std::data(types) + var_index,
                                                std::size(types) - var_index,
                                                b);
                for (size_t i = 0; i < vars.size(); ++i)
                    BinaryenFunctionSetLocalName(func, i, vars[i].name.c_str());
            }
            return func_t{mod, name, ret_type};
        }

        BinaryenType type(size_t index)
        {
            assert(index < vars.size() && "invalid local index");
            return types[index];
        }
        expr_ref get(size_t index)
        {
            assert(index < vars.size() && "invalid local index");
            return BinaryenLocalGet(mod, index, types[index]);
        }
        expr_ref set(size_t index, expr_ref value)
        {
            assert(index < vars.size() && "invalid local index");
            return BinaryenLocalSet(mod, index, value);
        }
        expr_ref tee(size_t index, expr_ref value)
        {
            assert(index < vars.size() && "invalid local index");
            return BinaryenLocalTee(mod, index, value, types[index]);
        }
    };

    function_stack::func_t compare(value_type vtype);

    const func_sig& require(functions function);

    auto call(functions function, expr_ref param)
    {
        auto& sig = require(function);
        return make_call(sig.name,
                         param,
                         sig.return_type);
    }

    auto call(functions function, nonstd::span<const expr_ref> params)
    {
        auto& sig = require(function);
        return make_call(sig.name,
                         params,
                         sig.return_type);
    }

    std::tuple<expr_ref_list, std::vector<size_t>> unpack_locals(function_stack& stack, nonstd::span<const char* const> p, expr_ref list)
    {
        size_t label_name              = 0;
        std::string none               = "+none" + std::to_string(label_name++);
        std::vector<const char*> names = {none.c_str()};
        std::vector<size_t> vars;
        expr_ref_list result;

        if (p.empty())
        {
            return {result, vars};
        }
        auto vararg    = "...";
        bool is_vararg = p.back() == std::string_view(vararg);
        if (is_vararg)
        {
            p = p.first(p.size() - 1);
        }
        for (auto arg : p)
        {
            names.push_back(arg);
            vars.push_back(stack.alloc(anyref(), arg));
        }

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

            auto new_array = stack.alloc(ref_array_type(), vararg);
            vars.push_back(new_array);

            auto size = stack.alloc(size_type(), "size");

            auto calc_size = local_tee(size, BinaryenBinary(mod, BinaryenSubInt32(), array_len(list), const_i32(p.size())), size_type());
            exp[1]         = BinaryenArrayCopy(mod,
                                       stack.tee(new_array,
                                                 BinaryenArrayNew(mod,
                                                                  BinaryenTypeGetHeapType(ref_array_type()),
                                                                  calc_size,
                                                                  nullptr)),
                                       const_i32(0),
                                       list,
                                       const_i32(p.size()),
                                       local_get(size, size_type()));
            stack.free_local(size);
        }

        bool first = !is_vararg;
        size_t j   = p.size();
        for (auto iter = p.rbegin(); iter != p.rend(); ++iter)
        {
            j--;
            exp[0] = BinaryenBlock(mod, *iter, std::data(exp), first ? 1 : std::size(exp), BinaryenTypeAuto());
            first  = false;

            auto get = array_get(
                list,
                const_i32(j),
                anyref());

            exp[1] = stack.set(vars[j], get);
        }

        result.push_back(make_block(exp, names[0]));
        return {result, vars};
    }

    expr_ref make_ref_array(function_stack& stack, nonstd::span<const expr_ref> p)
    {
        if (p.empty())
            return null();
        expr_ref_list result;
        size_t i = 0;
        for (auto exp : p)
        {
            i++;
            auto type = BinaryenExpressionGetType(exp);
            if (type == ref_array_type())
            {
                if (p.size() == 1)
                    return exp;

                auto local = stack.alloc(type, "i");

                exp = stack.tee(local, exp);
                if (i == p.size())
                {
                    auto new_array = stack.alloc(type, "new_array");

                    expr_ref_list copy;
                    copy.push_back(resize_array(new_array, type, stack.get(local), const_i32(result.size()), true));

                    size_t j = 0;
                    for (auto& init : result)
                        copy.push_back(ref_array::set(*this, local_get(new_array, type), const_i32(j++), init));

                    copy.push_back(local_get(new_array, type));

                    result.push_back(null());
                    stack.free_local(new_array);
                    stack.free_local(local);
                    return make_if(BinaryenRefIsNull(mod, exp),
                                   ref_array::create_fixed(*this, result),
                                   make_block(copy, "", type));
                }
                else
                {
                    exp = make_if(BinaryenRefIsNull(mod, exp),
                                  null(),
                                  ref_array::get(*this, stack.get(local), const_i32(0)));

                    result.push_back(exp);
                }
                stack.free_local(local);
            }
            else
                result.push_back(exp);
        }
        return ref_array::create_fixed(*this, result);
    }

    struct lua_std_func_t
    {
        runtime& self;
        expr_ref_list result;

        template<typename F, size_t N>
        void operator()(const char* name, const std::array<const char*, N>& args, F&& f)
        {
            result.push_back(self.add_lua_func(self.local_get(0, self.get_type<table>()), name, args, std::forward<F>(f)));
        }
    };

    template<typename F, size_t N>
    auto add_lua_func(expr_ref tbl, const char* name, const std::array<const char*, N>& arg_names, F&& f)
    {
        function_stack stack{mod};

        auto func = stack.add_function(name, ref_array_type(), [&](function_stack& stack)
                                       {
                                           stack.alloc(ref_array_type(), "upvalues");
                                           auto args = stack.alloc(ref_array_type(), "args");
                                           stack.locals();
                                           auto [result, vars] = unpack_locals(stack, arg_names, stack.get(args));
                                           std::array<size_t, N> vars_array;
                                           std::copy(vars.begin(), vars.end(), vars_array.begin());
                                           append(result, f(stack, vars_array));
                                           return make_block(result);
                                       });

        return call(functions::table_set,
                    std::array{
                        tbl,
                        add_string(name),
                        function::create(*this, std::array{func.get_ref(), null()}),
                    });
    }
};

} // namespace wumbo
