#include "lua2wasm.hpp"

#include "lua2wasm/compiler.hpp"
#include "lua2wasm/runtime.hpp"

namespace wumbo
{
wasm::mod compile(const block& chunk, uint32_t optimize)
{
    wasm::mod result;
    BinaryenModuleRef mod = reinterpret_cast<BinaryenModuleRef>(result.impl.get());
    runtime r{mod};
    r.create_functions = function_action::all;
    compiler c{mod, r};
    c.build_types();
    c.convert(chunk); // remove
    r.build();
    //c.convert();
    //BinaryenSetStart(mod, v);
    //BinaryenAddFunctionExport(mod, "convert", "start");
    //BinaryenModuleAutoDrop(mod);

    BinaryenModuleValidate(mod);

    if (optimize)
    {
        auto feature = BinaryenModuleGetFeatures(mod);
        auto res     = BinaryenModuleAllocateAndWrite(mod, nullptr);
        result.impl.reset(BinaryenModuleReadWithFeatures((char*)res.binary, res.binaryBytes, feature));
        mod = reinterpret_cast<BinaryenModuleRef>(result.impl.get());
        free(res.binary);
        free(res.sourceMap);
        BinaryenModuleOptimize(mod);
    }

    return result;
}

} // namespace wumbo
