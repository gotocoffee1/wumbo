#include "../runtime.hpp"

namespace wumbo
{

build_return_t runtime::open_os_lib()
{
    lua_std_func_t std{*this};

    std("clock", std::array<const char*, 0>{}, [this](runtime::function_stack& stack, auto&& vars) -> expr_ref
        {
            return BinaryenUnreachable(mod);
        });

    std("date", std::array{"format", "time"}, [this](runtime::function_stack& stack, auto&& vars) -> expr_ref
        {
            auto [format, time] = vars;
            return BinaryenUnreachable(mod);
        });

    std("difftime", std::array{"t2", "t1"}, [this](runtime::function_stack& stack, auto&& vars) -> expr_ref
        {
            auto [t2, t1] = vars;
            return BinaryenUnreachable(mod);
        });

    std("execute", std::array{"command"}, [this](runtime::function_stack& stack, auto&& vars) -> expr_ref
        {
            auto [command] = vars;
            return BinaryenUnreachable(mod);
        });

    std("exit", std::array{"code", "close"}, [this](runtime::function_stack& stack, auto&& vars) -> expr_ref
        {
            auto [code, close] = vars;
            return BinaryenUnreachable(mod);
        });

    std("getenv", std::array{"varname"}, [this](runtime::function_stack& stack, auto&& vars) -> expr_ref
        {
            auto [varname] = vars;
            return BinaryenUnreachable(mod);
        });

    std("remove", std::array{"filename"}, [this](runtime::function_stack& stack, auto&& vars) -> expr_ref
        {
            auto [filename] = vars;
            return BinaryenUnreachable(mod);
        });

    std("rename", std::array{"oldname", "newname"}, [this](runtime::function_stack& stack, auto&& vars) -> expr_ref
        {
            auto [oldname, newname] = vars;
            return BinaryenUnreachable(mod);
        });

    std("setlocale", std::array{"locale", "category"}, [this](runtime::function_stack& stack, auto&& vars) -> expr_ref
        {
            auto [locale, category] = vars;
            return BinaryenUnreachable(mod);
        });

    std("time", std::array{"table"}, [this](runtime::function_stack& stack, auto&& vars) -> expr_ref
        {
            auto [table] = vars;
            return BinaryenUnreachable(mod);
        });

    std("tmpname", std::array<const char*, 0>{}, [this](runtime::function_stack& stack, auto&& vars) -> expr_ref
        {
            return BinaryenUnreachable(mod);
        });

    std.result.push_back(local_get(0, get_type<table>()));

    return {std::vector<BinaryenType>{}, make_block(std.result)};
}
} // namespace wumbo
