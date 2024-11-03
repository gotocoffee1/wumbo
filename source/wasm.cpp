#include "wasm.hpp"

#include <ostream>

#include "binaryen-c.h"


void deleter::operator()(void* ptr)
{
    free(ptr);
}

result to_stream_bin(const wasm::mod& m)
{
    auto mod = reinterpret_cast<BinaryenModuleRef>(m.impl.get());
    auto res = BinaryenModuleAllocateAndWrite(mod, nullptr);
    return {std::unique_ptr<void, deleter>{res.binary},
            res.binaryBytes,
            std::unique_ptr<char, deleter>{res.sourceMap}};
}

void to_stream_bin(std::ostream& f, const wasm::mod& m)
{
    auto [data, size, source_map] = to_stream_bin(m);
    f.write(reinterpret_cast<char*>(data.get()), static_cast<std::streamsize>(size));
}

void to_stream_text(std::ostream& f, const wasm::mod& m)
{
    auto mod = reinterpret_cast<BinaryenModuleRef>(m.impl.get());
    auto res = BinaryenModuleAllocateAndWriteStackIR(mod);
    f << res;
    free(res);
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