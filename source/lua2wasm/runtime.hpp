#pragma once

#include "binaryen-c.h"
#include "wasm_util.hpp"

namespace wumbo
{
enum class functions
{
    table_get,
    table_set,
}; // namespace functions

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

    void build();
    build_return_t table_get();
    build_return_t table_set();
    BinaryenFunctionRef compare(const char* name, value_type vtype);

    const func_sig& require(functions function);

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
