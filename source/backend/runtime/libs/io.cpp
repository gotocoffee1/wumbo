#include "../runtime.hpp"

namespace wumbo
{

build_return_t runtime::open_io_lib()
{
    lua_std_func_t std{*this};

    std("close", std::array{"file"}, [this](runtime::function_stack& stack, auto&& vars) -> expr_ref
        {
            auto [file] = vars;
            return BinaryenUnreachable(mod);
        });

    std("flush", std::array<const char*, 0>{}, [this](runtime::function_stack& stack, auto&& vars) -> expr_ref
        {
            return BinaryenUnreachable(mod);
        });

    std("input", std::array{"file"}, [this](runtime::function_stack& stack, auto&& vars) -> expr_ref
        {
            auto [file] = vars;
            return BinaryenUnreachable(mod);
        });

    std("lines", std::array{"filename", "..."}, [this](runtime::function_stack& stack, auto&& vars) -> expr_ref
        {
            auto [filename, vaarg] = vars;
            return BinaryenUnreachable(mod);
        });

    std("open", std::array{"filename", "mode"}, [this](runtime::function_stack& stack, auto&& vars) -> expr_ref
        {
            auto [filename, mode] = vars;
            return BinaryenUnreachable(mod);
        });

    std("output", std::array{"file"}, [this](runtime::function_stack& stack, auto&& vars) -> expr_ref
        {
            auto [file] = vars;
            return BinaryenUnreachable(mod);
        });

    std("popen", std::array{"prog", "mode"}, [this](runtime::function_stack& stack, auto&& vars) -> expr_ref
        {
            auto [prog, mode] = vars;
            return BinaryenUnreachable(mod);
        });

    std("read", std::array{"..."}, [this](runtime::function_stack& stack, auto&& vars) -> expr_ref
        {
            auto [vaarg] = vars;
            return BinaryenUnreachable(mod);
        });

    std("tmpfile", std::array<const char*, 0>{}, [this](runtime::function_stack& stack, auto&& vars) -> expr_ref
        {
            return BinaryenUnreachable(mod);
        });

    std("type", std::array{"obj"}, [this](runtime::function_stack& stack, auto&& vars) -> expr_ref
        {
            auto [obj] = vars;
            return BinaryenUnreachable(mod);
        });

    std("write", std::array{"..."}, [this](runtime::function_stack& stack, auto&& vars) -> expr_ref
        {
            auto [vaarg] = vars;
            return BinaryenUnreachable(mod);
        });
    std.result.push_back(local_get(0, get_type<table>()));

    return {std::vector<BinaryenType>{}, make_block(std.result)};
}
} // namespace wumbo
