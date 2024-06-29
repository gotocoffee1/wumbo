#include "wasm.hpp"

#include <ostream>

#include "binaryen-c.h"

void to_stream_bin(std::ostream& f, const wasm::mod& m)
{
    auto mod = reinterpret_cast<BinaryenModuleRef>(m.impl.get());
    auto res = BinaryenModuleAllocateAndWrite(mod, nullptr);
    f.write(reinterpret_cast<char*>(res.binary), static_cast<std::streamsize>(res.binaryBytes));
    free(res.binary);
    free(res.sourceMap);
}

void to_stream_text(std::ostream& f, const wasm::mod& m)
{
    auto mod = reinterpret_cast<BinaryenModuleRef>(m.impl.get());
    auto res = BinaryenModuleAllocateAndWriteText(mod);
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