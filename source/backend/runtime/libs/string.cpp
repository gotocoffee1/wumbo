#include "../runtime.hpp"

namespace wumbo
{

build_return_t runtime::open_string_lib()
{
    lua_std_func_t std{*this};

    std("byte", std::array{"s", "i", "j"}, [this](runtime::function_stack& stack, auto&& vars) -> expr_ref
        {
            auto [s, i, j] = vars;
            return BinaryenUnreachable(mod);
        });

    std("char", std::array{"..."}, [this](runtime::function_stack& stack, auto&& vars) -> expr_ref
        {
            return BinaryenUnreachable(mod);
        });

    std("dump", std::array{"func", "strip"}, [this](runtime::function_stack& stack, auto&& vars) -> expr_ref
        {
            auto [func, strip] = vars;
            return BinaryenUnreachable(mod);
        });

    std("find", std::array{"s", "pattern", "init", "plain"}, [this](runtime::function_stack& stack, auto&& vars) -> expr_ref
        {
            auto [s, pattern, init, plain] = vars;
            return BinaryenUnreachable(mod);
        });

    std("format", std::array{"formatstring", "..."}, [this](runtime::function_stack& stack, auto&& vars) -> expr_ref
        {
            return BinaryenUnreachable(mod);
        });

    std("gmatch", std::array{"s", "pattern"}, [this](runtime::function_stack& stack, auto&& vars) -> expr_ref
        {
            auto [s, pattern] = vars;
            return BinaryenUnreachable(mod);
        });

    std("gsub", std::array{"s", "pattern", "repl", "n"}, [this](runtime::function_stack& stack, auto&& vars) -> expr_ref
        {
            auto [s, pattern, repl, n] = vars;
            return BinaryenUnreachable(mod);
        });

    std("len", std::array{"s"}, [this](runtime::function_stack& stack, auto&& vars) -> expr_ref
        {
            auto [s] = vars;
            return BinaryenUnreachable(mod);
        });

    std("lower", std::array{"s"}, [this](runtime::function_stack& stack, auto&& vars) -> expr_ref
        {
            auto [s] = vars;
            return BinaryenUnreachable(mod);
        });

    std("match", std::array{"s", "pattern", "init"}, [this](runtime::function_stack& stack, auto&& vars) -> expr_ref
        {
            auto [s, pattern, init] = vars;
            return BinaryenUnreachable(mod);
        });

    std("pack", std::array{"fmt", "v1", "v2", "..."}, [this](runtime::function_stack& stack, auto&& vars) -> expr_ref
        {
            return BinaryenUnreachable(mod);
        });

    std("packsize", std::array{"fmt"}, [this](runtime::function_stack& stack, auto&& vars) -> expr_ref
        {
            auto [fmt] = vars;
            return BinaryenUnreachable(mod);
        });

    std("rep", std::array{"s", "n", "sep"}, [this](runtime::function_stack& stack, auto&& vars) -> expr_ref
        {
            auto [s, n, sep] = vars;
            return BinaryenUnreachable(mod);
        });

    std("reverse", std::array{"s"}, [this](runtime::function_stack& stack, auto&& vars) -> expr_ref
        {
            auto [s] = vars;
            return BinaryenUnreachable(mod);
        });

    std("sub", std::array{"s", "i", "j"}, [this](runtime::function_stack& stack, auto&& vars) -> expr_ref
        {
            auto [s, i, j] = vars;
            return BinaryenUnreachable(mod);
        });

    std("unpack", std::array{"fmt", "s", "pos"}, [this](runtime::function_stack& stack, auto&& vars) -> expr_ref
        {
            auto [fmt, s, pos] = vars;
            return BinaryenUnreachable(mod);
        });

    std("upper", std::array{"s"}, [this](runtime::function_stack& stack, auto&& vars) -> expr_ref
        {
            auto [s] = vars;
            return BinaryenUnreachable(mod);
        });

    std.result.push_back(local_get(0, get_type<table>()));

    return {std::vector<BinaryenType>{}, make_block(std.result)};
}
} // namespace wumbo
