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

    auto start = c.add_func("*init", chunk, {}, true);
    c.convert();
    //BinaryenSetStart(mod, v);
    BinaryenAddFunctionExport(mod, "convert", "start");
    //BinaryenModuleAutoDrop(mod);
    BinaryenModuleValidate(mod);
    //BinaryenModuleInterpret(mod);
    //BinaryenModuleOptimize(mod);
    //BinaryenModuleSetTypeName(mod, builtHeapTypes[0], "SomeStruct");
    //BinaryenModuleSetFieldName(mod, builtHeapTypes[0], 0, "SomeField");
    return result;
}

} // namespace wumbo
