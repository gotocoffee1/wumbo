#include "lua2wasm.hpp"

#include "lua2wasm/compiler.hpp"

namespace wumbo
{
wasm::mod compile(const block& chunk)
{
    wasm::mod result;
    BinaryenModuleRef mod = reinterpret_cast<BinaryenModuleRef>(result.impl.get());
    compiler c{mod};
    c.build_types();
    c.convert(chunk); // remove

    //c.convert();
    //BinaryenSetStart(mod, v);
    //BinaryenAddFunctionExport(mod, "convert", "start");
    //BinaryenModuleAutoDrop(mod);

    BinaryenModuleValidate(mod);

    auto feature = BinaryenModuleGetFeatures(mod);
    auto res = BinaryenModuleAllocateAndWrite(mod, nullptr);
    result.impl.reset(BinaryenModuleReadWithFeatures((char*)res.binary, res.binaryBytes, feature));
    mod = reinterpret_cast<BinaryenModuleRef>(result.impl.get());
    free(res.binary);
    free(res.sourceMap);
    BinaryenModuleOptimize(mod);

    return result;
}

} // namespace wumbo
