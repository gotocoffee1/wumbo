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
    //BinaryenModuleInterpret(mod);
    //BinaryenModuleOptimize(mod);
    return result;
}

} // namespace wumbo
