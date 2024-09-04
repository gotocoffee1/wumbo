#pragma once

#include "ast.hpp"
#include "wasm.hpp"

namespace wumbo
{
wasm::mod compile(const ast::block& chunk);
} // namespace wumbo