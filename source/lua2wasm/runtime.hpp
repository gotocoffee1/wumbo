#pragma once

#include "binaryen-c.h"
#include "wasm_util.hpp"
#include <type_traits>

#define RUNTIME_FUNCTIONS(DO)                                                    \
    DO(table_get, create_type(anyref(), anyref()), anyref())                     \
    DO(table_set, create_type(anyref(), anyref(), anyref()), BinaryenTypeNone()) \
    DO(to_bool, anyref(), bool_type())                                           \
    DO(to_bool_not, anyref(), bool_type())

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

    template<size_t N>
    auto call(functions function, std::array<expr_ref, N> params)
    {
        auto& sig = require(function);
        return make_call(sig.name,
                         params,
                         sig.return_type);
    }
};

} // namespace wumbo
