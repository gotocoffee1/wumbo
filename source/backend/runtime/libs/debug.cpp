#include "../runtime.hpp"

namespace wumbo
{

build_return_t runtime::open_debug_lib()
{
    lua_std_func_t std{*this};

    std("debug", std::array<const char*, 0>{}, [this](runtime::function_stack& stack, auto&& vars) -> expr_ref
        {
            return BinaryenUnreachable(mod);
        });

    std("gethook", std::array{"thread"}, [this](runtime::function_stack& stack, auto&& vars) -> expr_ref
        {
            auto [thread] = vars;
            return BinaryenUnreachable(mod);
        });

    std("getinfo", std::array{"thread", "f", "what"}, [this](runtime::function_stack& stack, auto&& vars) -> expr_ref
        {
            auto [thread, f, what] = vars;
            return BinaryenUnreachable(mod);
        });

    std("getlocal", std::array{"thread", "level", "idx"}, [this](runtime::function_stack& stack, auto&& vars) -> expr_ref
        {
            auto [thread, level, idx] = vars;
            return BinaryenUnreachable(mod);
        });

    std("getmetatable", std::array{"value"}, [this](runtime::function_stack& stack, auto&& vars) -> expr_ref
        {
            auto [value] = vars;
            return BinaryenUnreachable(mod);
        });

    std("getregistry", std::array<const char*, 0>{}, [this](runtime::function_stack& stack, auto&& vars) -> expr_ref
        {
            return BinaryenUnreachable(mod);
        });

    std("getupvalue", std::array{"f", "idx"}, [this](runtime::function_stack& stack, auto&& vars) -> expr_ref
        {
            auto [f, idx] = vars;
            return BinaryenUnreachable(mod);
        });

    std("getuservalue", std::array{"u"}, [this](runtime::function_stack& stack, auto&& vars) -> expr_ref
        {
            auto [u] = vars;
            return BinaryenUnreachable(mod);
        });

    std("sethook", std::array{"thread", "hook", "mask", "count"}, [this](runtime::function_stack& stack, auto&& vars) -> expr_ref
        {
            auto [thread, hook, mask, count] = vars;
            return BinaryenUnreachable(mod);
        });

    std("setlocal", std::array{"thread", "level", "idx", "value"}, [this](runtime::function_stack& stack, auto&& vars) -> expr_ref
        {
            auto [thread, level, idx, value] = vars;
            return BinaryenUnreachable(mod);
        });

    std("setmetatable", std::array{"value", "table"}, [this](runtime::function_stack& stack, auto&& vars) -> expr_ref
        {
            auto [value, table] = vars;
            return BinaryenUnreachable(mod);
        });

    std("setupvalue", std::array{"f", "up", "value"}, [this](runtime::function_stack& stack, auto&& vars) -> expr_ref
        {
            auto [f, up, value] = vars;
            return BinaryenUnreachable(mod);
        });

    std("setuservalue", std::array{"udata", "value"}, [this](runtime::function_stack& stack, auto&& vars) -> expr_ref
        {
            auto [udata, value] = vars;
            return BinaryenUnreachable(mod);
        });

    std("traceback", std::array{"thread", "message", "level"}, [this](runtime::function_stack& stack, auto&& vars) -> expr_ref
        {
            auto [thread, message, level] = vars;
            return BinaryenUnreachable(mod);
        });

    std("upvalueid", std::array{"f", "n"}, [this](runtime::function_stack& stack, auto&& vars) -> expr_ref
        {
            auto [f, n] = vars;
            return BinaryenUnreachable(mod);
        });

    std("upvaluejoin", std::array{"f1", "n1", "f2", "n2"}, [this](runtime::function_stack& stack, auto&& vars) -> expr_ref
        {
            auto [f1, n1, f2, n2] = vars;
            return BinaryenUnreachable(mod);
        });

    std.result.push_back(local_get(0, get_type<table>()));

    return {std::vector<BinaryenType>{}, make_block(std.result)};
}
} // namespace wumbo
