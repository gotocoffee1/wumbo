#include "lua2wasm.hpp"

#include "lua2wasm/compiler.hpp"

namespace wumbo
{

void opt(wasm::mod& result)
{
    BinaryenModuleRef mod = reinterpret_cast<BinaryenModuleRef>(result.impl.get());
    auto feature = BinaryenModuleGetFeatures(mod);
    auto res     = BinaryenModuleAllocateAndWrite(mod, nullptr);
    result.impl.reset(BinaryenModuleReadWithFeatures((char*)res.binary, res.binaryBytes, feature));
    mod = reinterpret_cast<BinaryenModuleRef>(result.impl.get());
    free(res.binary);
    free(res.sourceMap);
    BinaryenModuleOptimize(mod);
}

wasm::mod make_runtime(uint32_t optimize)
{
    wasm::mod result;
    BinaryenModuleRef mod = reinterpret_cast<BinaryenModuleRef>(result.impl.get());
    runtime r{mod};

    r.create_functions = function_action::all;
    r.export_functions = function_action::all;
    r.import_functions = function_action::none;
    r.build_types();
    r.build();

    BinaryenModuleValidate(mod);
    if (optimize)
        opt(result);

    return result;
}

wasm::mod compile(const block& chunk, uint32_t optimize, bool stand_alone)
{
    wasm::mod result;
    BinaryenModuleRef mod = reinterpret_cast<BinaryenModuleRef>(result.impl.get());
    runtime r{mod};

    if (!stand_alone)
    {
        r.create_functions = function_action::none;
        r.export_functions = function_action::none;
        r.import_functions = function_action::required;
    }
    else
    {
        r.create_functions = function_action::all;
        r.export_functions = function_action::none;
        r.import_functions = function_action::none;
    }
    r.build_types();
    compiler c{mod, r};
    c.convert(chunk); // remove
    r.build();

    //BinaryenSetStart(mod, v);
    //BinaryenAddFunctionExport(mod, "convert", "start");
    //BinaryenModuleAutoDrop(mod);

    BinaryenModuleValidate(mod);

    if (optimize)
    {
        opt(result);
    }

    return result;
}

} // namespace wumbo
