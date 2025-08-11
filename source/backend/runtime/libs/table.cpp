#include "../runtime.hpp"

namespace wumbo
{
build_return_t runtime::open_table_lib()
{
    expr_ref_list result;
    auto add_func = [&](const char* name, const std::vector<std::string>& args, bool vararg, auto&& f)
    {
        result.push_back(add_lua_func(local_get(0, get_type<table>()), name, args, vararg, f));
    };
    add_func("concat", {"self", "key", "value"}, false, [this](runtime::function_stack& stack) -> expr_ref
             {
                 return BinaryenUnreachable(mod);
             });

    return {std::vector<BinaryenType>{}, make_block(result)};
}
} // namespace wumbo
