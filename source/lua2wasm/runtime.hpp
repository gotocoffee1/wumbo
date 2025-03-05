#pragma once

#include "wasm_util.hpp"

namespace wumbo
{
struct runtime : ext_types
{
    void build();
    void func_table_get();
    BinaryenFunctionRef compare(const char* name, value_type vtype);
};
}

