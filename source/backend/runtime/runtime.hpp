#pragma once

#include "backend/wasm_util.hpp"
#include "binaryen-c.h"
#include <type_traits>

#define RUNTIME_FUNCTIONS(DO)                                                                   \
    DO(table_get, create_type(anyref(), anyref()), anyref())                                    \
    DO(table_set, create_type(anyref(), anyref(), anyref()), BinaryenTypeNone())                \
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
    DO(get_type, anyref(), size_type())                                                         \
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

#define DECL_FUNCS(name, ...) build_return_t name();
    RUNTIME_FUNCTIONS(DECL_FUNCS)
#undef DECL_FUNCS

    BinaryenFunctionRef compare(const char* name, value_type vtype);

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
