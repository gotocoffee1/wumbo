#include "../runtime.hpp"

namespace wumbo
{

build_return_t runtime::open_os_lib()
{
    lua_std_func_t std{*this};

    std("concat", std::array{"list", "sep", "i", "j"}, [this](runtime::function_stack& stack, auto&& vars) -> expr_ref
             {
                 auto [list, sep, i, j] = vars;
                 return BinaryenUnreachable(mod);
             });

    std.result.push_back(local_get(0, get_type<table>()));

    return {std::vector<BinaryenType>{}, make_block(std.result)};
}
} // namespace wumbo
