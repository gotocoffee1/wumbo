#pragma once

#include "ast.hpp"
#include "wasm.hpp"

namespace wumbo
{
wasm::mod compile(const ast::block& chunk, uint32_t optimize);
} // namespace wumbo