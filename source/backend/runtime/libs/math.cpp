#include "../runtime.hpp"

namespace wumbo
{

build_return_t runtime::open_math_lib()
{
    lua_std_func_t std{*this};

    std("abs", std::array{"x"}, [this](runtime::function_stack& stack, auto&& vars) -> expr_ref
        {
            auto [x] = vars;
            return BinaryenUnreachable(mod);
        });

    std("acos", std::array{"x"}, [this](runtime::function_stack& stack, auto&& vars) -> expr_ref
        {
            auto [x] = vars;
            return BinaryenUnreachable(mod);
        });

    std("asin", std::array{"x"}, [this](runtime::function_stack& stack, auto&& vars) -> expr_ref
        {
            auto [x] = vars;
            return BinaryenUnreachable(mod);
        });

    std("atan", std::array{"y", "x"}, [this](runtime::function_stack& stack, auto&& vars) -> expr_ref
        {
            auto [y, x] = vars;
            return BinaryenUnreachable(mod);
        });

    std("ceil", std::array{"x"}, [this](runtime::function_stack& stack, auto&& vars) -> expr_ref
        {
            auto [x] = vars;
            return BinaryenUnreachable(mod);
        });

    std("cos", std::array{"x"}, [this](runtime::function_stack& stack, auto&& vars) -> expr_ref
        {
            auto [x] = vars;
            return BinaryenUnreachable(mod);
        });

    std("deg", std::array{"x"}, [this](runtime::function_stack& stack, auto&& vars) -> expr_ref
        {
            auto [x] = vars;
            return BinaryenUnreachable(mod);
        });

    std("exp", std::array{"x"}, [this](runtime::function_stack& stack, auto&& vars) -> expr_ref
        {
            auto [x] = vars;
            return BinaryenUnreachable(mod);
        });

    std("floor", std::array{"x"}, [this](runtime::function_stack& stack, auto&& vars) -> expr_ref
        {
            auto [x] = vars;
            return BinaryenUnreachable(mod);
        });

    std("fmod", std::array{"x", "y"}, [this](runtime::function_stack& stack, auto&& vars) -> expr_ref
        {
            auto [x, y] = vars;
            return BinaryenUnreachable(mod);
        });

    std.set("huge", add_string("TODO"));

    std("log", std::array{"x", "base"}, [this](runtime::function_stack& stack, auto&& vars) -> expr_ref
        {
            auto [x, base] = vars;
            return BinaryenUnreachable(mod);
        });

    std("max", std::array{"x", "..."}, [this](runtime::function_stack& stack, auto&& vars) -> expr_ref
        {
            auto [x, vaarg] = vars;
            return BinaryenUnreachable(mod);
        });

    std.set("maxinteger", add_string("TODO"));

    std("min", std::array{"x", "..."}, [this](runtime::function_stack& stack, auto&& vars) -> expr_ref
        {
            auto [x, vaarg] = vars;
            return BinaryenUnreachable(mod);
        });

    std.set("mininteger", add_string("TODO"));

    std("modf", std::array{"x"}, [this](runtime::function_stack& stack, auto&& vars) -> expr_ref
        {
            auto [x] = vars;
            return BinaryenUnreachable(mod);
        });

    std.set("pi", add_string("TODO"));

    std("rad", std::array{"x"}, [this](runtime::function_stack& stack, auto&& vars) -> expr_ref
        {
            auto [x] = vars;
            return BinaryenUnreachable(mod);
        });

    std("random", std::array{"m", "n"}, [this](runtime::function_stack& stack, auto&& vars) -> expr_ref
        {
            auto [m, n] = vars;
            return BinaryenUnreachable(mod);
        });

    std("randomseed", std::array{"x"}, [this](runtime::function_stack& stack, auto&& vars) -> expr_ref
        {
            auto [x] = vars;
            return BinaryenUnreachable(mod);
        });

    std("sin", std::array{"x"}, [this](runtime::function_stack& stack, auto&& vars) -> expr_ref
        {
            auto [x] = vars;
            return BinaryenUnreachable(mod);
        });

    std("sqrt", std::array{"x"}, [this](runtime::function_stack& stack, auto&& vars) -> expr_ref
        {
            auto [x] = vars;
            return BinaryenUnreachable(mod);
        });

    std("tan", std::array{"x"}, [this](runtime::function_stack& stack, auto&& vars) -> expr_ref
        {
            auto [x] = vars;
            return BinaryenUnreachable(mod);
        });

    std("tointeger", std::array{"x"}, [this](runtime::function_stack& stack, auto&& vars) -> expr_ref
        {
            auto [x] = vars;
            return BinaryenUnreachable(mod);
        });

    std("type", std::array{"x"}, [this](runtime::function_stack& stack, auto&& vars) -> expr_ref
        {
            auto [x] = vars;
            return BinaryenUnreachable(mod);
        });

    std("ult", std::array{"m", "n"}, [this](runtime::function_stack& stack, auto&& vars) -> expr_ref
        {
            auto [m, n] = vars;
            return BinaryenUnreachable(mod);
        });

    std.result.push_back(local_get(0, get_type<table>()));

    return {std::vector<BinaryenType>{}, make_block(std.result)};
}
} // namespace wumbo
