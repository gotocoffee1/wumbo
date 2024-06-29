#pragma once

#include "ast.hpp"
#include "wasm.hpp"

namespace lua2wasm
{
wasm::mod compile(const ast::block& chunk);
} // namespace lua2wasm