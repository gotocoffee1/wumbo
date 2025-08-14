#include "../runtime.hpp"

namespace wumbo
{

build_return_t runtime::open_debug_lib()
{
    expr_ref_list result;
    auto add_func = [&](const char* name, auto&& args, auto&& f)
    {
        result.push_back(add_lua_func(local_get(0, get_type<table>()), name, args, f));
    };
    add_func("concat", std::array{"list", "sep", "i", "j"}, [this](runtime::function_stack& stack, auto&& vars) -> expr_ref
             {
                 auto [list, sep, i, j] = vars;
                 return BinaryenUnreachable(mod);
             });

    result.push_back(local_get(0, get_type<table>()));

    return {std::vector<BinaryenType>{}, make_block(result)};
}
} // namespace wumbo
