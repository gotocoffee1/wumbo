#pragma once

#include <memory>



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
    void operator()(void*);
};


struct result
{
    std::unique_ptr<void, deleter> data;
    size_t size;
    std::unique_ptr<char, deleter>  source_map;
    std::unique_ptr<char, deleter>  wat;
};

enum class wat
{
    none = 0,
    stack = 1,
    function = 2,
};

result to_stream_bin(const wasm::mod& m, wat mode);
