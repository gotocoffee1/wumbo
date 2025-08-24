#include "../runtime.hpp"

namespace wumbo
{

build_return_t runtime::open_coroutine_lib()
{
    lua_std_func_t std{*this};

    std("create", std::array{"f"}, [this](runtime::function_stack& stack, auto&& vars) -> expr_ref
        {
            auto [f] = vars;
            return BinaryenUnreachable(mod);
        });

    std("isyieldable", std::array<const char*, 0>{}, [this](runtime::function_stack& stack, auto&& vars) -> expr_ref
        {
            return BinaryenUnreachable(mod);
        });

    std("resume", std::array{"co", "val", "..."}, [this](runtime::function_stack& stack, auto&& vars) -> expr_ref
        {
            auto [co, val, vaarg] = vars;
            return BinaryenUnreachable(mod);
        });

    std("running", std::array<const char*, 0>{}, [this](runtime::function_stack& stack, auto&& vars) -> expr_ref
        {
            return BinaryenUnreachable(mod);
        });

    std("status", std::array{"co"}, [this](runtime::function_stack& stack, auto&& vars) -> expr_ref
        {
            auto [co] = vars;
            return BinaryenUnreachable(mod);
        });

    std("wrap", std::array{"f"}, [this](runtime::function_stack& stack, auto&& vars) -> expr_ref
        {
            auto [f] = vars;
            return BinaryenUnreachable(mod);
        });

    std("yield", std::array{"..."}, [this](runtime::function_stack& stack, auto&& vars) -> expr_ref
        {
            auto [vaarg] = vars;
            return BinaryenUnreachable(mod);
        });

    std.result.push_back(local_get(0, get_type<table>()));

    return {std::vector<BinaryenType>{}, make_block(std.result)};
}
} // namespace wumbo
