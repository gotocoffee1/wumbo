#pragma once

#include "ast/ast.hpp"
#include "backend/wasm.hpp"

namespace wumbo
{
wasm::mod make_runtime(uint32_t optimize);

wasm::mod compile(const ast::block& chunk, uint32_t optimize, bool standalone);
} // namespace wumbo
