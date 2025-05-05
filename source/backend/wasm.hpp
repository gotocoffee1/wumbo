#pragma once

#include <memory>

namespace wumbo
{
namespace wasm
{

struct mod
{
    mod();
    struct deleter
    {
        void operator()(void*);
    };
    std::unique_ptr<void, deleter> impl;
};

} // namespace wasm

struct deleter
{
    void operator()(const void*);
};

enum class wat
{
    function = 0,
    stack    = 1,
};


using c_str = std::unique_ptr<const char, deleter>;

std::tuple<
    std::unique_ptr<const void, deleter>,
    size_t,
    c_str> to_bin(const wasm::mod& m);

c_str to_txt(const wasm::mod& m, wat mode);
} // namespace wumbo
