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
        };

        template<typename F>
        func_t add_function(const std::string& name, BinaryenType ret_type, F&& body)
        {
            if (!BinaryenGetFunction(mod, name.c_str()))
            {
                auto b    = body();
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
};

} // namespace wumbo
