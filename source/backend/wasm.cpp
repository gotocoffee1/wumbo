#include "wasm.hpp"

#include <ostream>

#include "binaryen-c.h"

namespace wumbo
{

void deleter::operator()(const void* ptr)
{
    free(const_cast<void*>(ptr));
}

std::tuple<
    std::unique_ptr<const void, deleter>,
    size_t,
    c_str>
to_bin(const wasm::mod& m)
{
    auto mod = reinterpret_cast<BinaryenModuleRef>(m.impl.get());
    auto res = BinaryenModuleAllocateAndWrite(mod, nullptr);

    return {
        std::unique_ptr<void, deleter>{res.binary},
        res.binaryBytes,
        c_str{res.sourceMap},
    };
}

c_str to_txt(const wasm::mod& m, wat mode)
{
    auto mod = reinterpret_cast<BinaryenModuleRef>(m.impl.get());
    const char* txt;
    switch (mode)
    {
    case wat::stack:
        txt = BinaryenModuleAllocateAndWriteStackIR(mod);
        break;
    case wat::function:
        txt = BinaryenModuleAllocateAndWriteText(mod);
        break;
    default:
        return nullptr;
    }
    return c_str{txt};
}

void to_stream_bin(std::ostream& f, const wasm::mod& m)
{
    auto [data, size, source_map] = to_bin(m);
    f.write(reinterpret_cast<const char*>(data.get()), static_cast<std::streamsize>(size));
}

void to_stream_text(std::ostream& f, const wasm::mod& m)
{
    auto txt = to_txt(m, wat::stack);
    f << txt.get();
}

namespace wasm
{

mod::mod()
{
    impl.reset(BinaryenModuleCreate());
}

void mod::deleter::operator()(void* ptr)
{
    BinaryenModuleDispose(reinterpret_cast<BinaryenModuleRef>(ptr));
}
} // namespace wasm
} // namespace wumbo
