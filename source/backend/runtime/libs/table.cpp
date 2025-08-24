#include "../runtime.hpp"

namespace wumbo
{

build_return_t runtime::open_table_lib()
{
    lua_std_func_t std{*this};

    std("concat", std::array{"list", "sep", "i", "j"}, [this](runtime::function_stack& stack, auto&& vars) -> expr_ref
        {
            auto [list, sep, i, j] = vars;
            return BinaryenUnreachable(mod);
        });

    std("insert", std::array{"list", "pos", "value"}, [this](runtime::function_stack& stack, auto&& vars) -> expr_ref
        {
            auto [list, pos, value] = vars;
            return BinaryenUnreachable(mod);
        });

    std("move", std::array{"a1", "f", "e", "t", "a2"}, [this](runtime::function_stack& stack, auto&& vars) -> expr_ref
        {
            auto [a1, f, e, t, a2] = vars;
            return BinaryenUnreachable(mod);
        });

    std("pack", std::array{"..."}, [this](runtime::function_stack& stack, auto&& vars) -> expr_ref
        {
            return BinaryenUnreachable(mod);
        });

    std("remove", std::array{"list", "pos"}, [this](runtime::function_stack& stack, auto&& vars) -> expr_ref
        {
            auto [list, pos] = vars;
            return BinaryenUnreachable(mod);
        });

    std("sort", std::array{"list", "comp"}, [this](runtime::function_stack& stack, auto&& vars) -> expr_ref
        {
            auto [list, comp] = vars;
            return BinaryenUnreachable(mod);
        });

    std("unpack", std::array{"list", "i", "j"}, [this](runtime::function_stack& stack, auto&& vars) -> expr_ref
        {
            auto [list, i, j] = vars;
            return BinaryenUnreachable(mod);
        });

    std.result.push_back(local_get(0, get_type<table>()));

    return {std::vector<BinaryenType>{}, make_block(std.result)};
}
} // namespace wumbo
