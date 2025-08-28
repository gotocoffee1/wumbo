#include "../runtime.hpp"

namespace wumbo
{

build_return_t runtime::open_utf8_lib()
{
    lua_std_func_t std{*this, "utf8"};

    std("char", std::array{"..."}, [this](runtime::function_stack& stack, auto&& vars) -> expr_ref
        {
            return BinaryenUnreachable(mod);
        });

    std.set("charpattern", add_string("[\0-\x7F\xC2-\xF4][\x80-\xBF]*"));

    std("codes", std::array{"s"}, [this](runtime::function_stack& stack, auto&& vars) -> expr_ref
        {
            auto [s] = vars;
            return BinaryenUnreachable(mod);
        });

    std("codepoint", std::array{"s", "i", "j"}, [this](runtime::function_stack& stack, auto&& vars) -> expr_ref
        {
            auto [s, i, j] = vars;
            return BinaryenUnreachable(mod);
        });

    std("len", std::array{"s", "i", "j"}, [this](runtime::function_stack& stack, auto&& vars) -> expr_ref
        {
            auto [s, i, j] = vars;
            return BinaryenUnreachable(mod);
        });

    std("offset", std::array{"s", "n", "i"}, [this](runtime::function_stack& stack, auto&& vars) -> expr_ref
        {
            auto [s, n, i] = vars;
            return BinaryenUnreachable(mod);
        });

    std.result.push_back(local_get(0, get_type<table>()));

    return {std::vector<BinaryenType>{}, make_block(std.result)};
}
} // namespace wumbo
