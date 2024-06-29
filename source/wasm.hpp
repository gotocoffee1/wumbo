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
