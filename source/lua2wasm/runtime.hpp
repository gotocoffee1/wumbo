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

using build_return_t = std::tuple<std::vector<BinaryenType>, expr_ref>; 
struct runtime : ext_types
{
    bool export_functions = false;
    bool import_functions = false;
    bool create_functions = false;
    bool export_required_functions = false;
    bool import_required_functions = false;
    bool create_required_functions = false;
    void build();
    build_return_t table_get();
    BinaryenFunctionRef compare(const char* name, value_type vtype);
    std::vector<bool> _required_functions;
};

using build_func_t = build_return_t (runtime::*)();

struct func_sig
{
    const char* name;
    BinaryenType params_type;
    BinaryenType return_type;
    build_func_t build;
};

} // namespace wumbo
