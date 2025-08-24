#include "../runtime.hpp"

namespace wumbo
{

build_return_t runtime::open_package_lib()
{
    lua_std_func_t std{*this};

    std.set("config", add_string("TODO"));
    std.set("cpath", add_string("TODO"));
    std.set("loaded", add_string("TODO"));
    std.set("path", add_string("TODO"));
    std.set("preload", add_string("TODO"));
    std.set("searchers", add_string("TODO"));

    std("searchpath", std::array{"name", "path", "sep", "rep"}, [this](runtime::function_stack& stack, auto&& vars) -> expr_ref
        {
            auto [name, path, sep, rep] = vars;
            return BinaryenUnreachable(mod);
        });

    std.result.push_back(local_get(0, get_type<table>()));

    return {std::vector<BinaryenType>{}, make_block(std.result)};
}
} // namespace wumbo