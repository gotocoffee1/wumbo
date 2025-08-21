#include "../runtime.hpp"

namespace wumbo
{

build_return_t runtime::open_basic_lib()
{
    lua_std_func_t std{*this};
    BinaryenAddFunctionImport(mod, "stdout", "native", "stdout", BinaryenTypeExternref(), BinaryenTypeNone());
    BinaryenAddFunctionImport(mod, "load_lua", "load", "load", BinaryenTypeExternref(), lua_func());

    // std("assert", std::arraystd::array{"v" /*, "message"*/}, [this](runtime::function_stack& stack, auto&& vars)
    //          {
    //              auto [v] = vars;

    //              auto alt = [&]()
    //              {
    //                  return add_string("assertion failed!");
    //              };
    //              return make_if(call(functions::to_bool, stack.get(v), make_return(local_get(args_index, ref_array_type())), throw_error(at_or_null(args_index, 1, nullptr, alt))));
    //          });
    std("collectgarbage", std::array{"opt", "arg"}, [this](function_stack& stack, auto&& vars)
        {
            auto [opt, arg] = vars;
            return null();
        });

    std("dofile", std::array{"filename"}, [this](function_stack& stack, auto&& vars)
        {
            return BinaryenUnreachable(mod);
        });
    std("error", std::array{"message", "level"}, [this](function_stack& stack, auto&& vars)
        {
            auto [message, level] = vars;
            return throw_error(stack.get(message));
        });
    std("getmetatable", std::array{"object"}, [this](function_stack& stack, auto&& vars)
        {
            return BinaryenUnreachable(mod);
        });
    std("ipairs", std::array{"t"}, [this](function_stack& stack, auto&& vars)
        {
            return BinaryenUnreachable(mod);
        });
    /*
    std("load", std::array{"chunk", "chunkname", "mode", "env"}, false, [&]()
             {
                 auto chunk = get_var("chunk");

                 auto casts = std::array{
                     value_type::string,
                     //value_type::function,
                 };

                 return switch_value(chunk, casts, [this](value_type type, expr_ref exp)
                                     {
                                         switch (type)
                                         {
                                         case value_type::string:
                                             exp = _runtime.call(functions::lua_str_to_js_array, exp);
                                             exp = make_call("load_lua", exp, lua_func());
                                             exp = build_closure(exp, {get_var("_ENV")});
                                             break;
                                         case value_type::function:
                                         default:
                                             return BinaryenUnreachable(mod);
                                         }
                                         return exp ? make_return(make_ref_array(exp)) : make_return(null());
                                     });
             });*/
    std("loadfile", std::array{"filename ", "mode", "env"}, [this](function_stack& stack, auto&& vars)
        {
            return BinaryenUnreachable(mod);
        });
    std("next", std::array{"table", "index"}, [this](function_stack& stack, auto&& vars)
        {
            return BinaryenUnreachable(mod);
        });
    // std("pairs", std::array{"t"}, [this](function_stack& stack, auto&& vars)
    //     {
    //         return BinaryenUnreachable(mod);
    //     });

    auto make_pcall = [this](bool x)
    {
        return [this, x](function_stack& stack, auto&& vars)
        {
            auto f = vars[0];

            auto args = vars[1 + x];

            auto exception = stack.alloc(anyref(), "exception");

            const char* tags[] = {error_tag};
            expr_ref catches[] = {
                x ? make_block(std::array{
                        stack.set(exception, BinaryenPop(mod, anyref())),
                        drop(call(functions::invoke, std::array{stack.get(vars[1]), ref_array::create_fixed(*this, stack.get(exception))})),
                        make_return(make_ref_array(stack, std::array{new_boolean(const_boolean(false)), stack.get(exception)})),
                    })
                  : make_block(std::array{
                        stack.set(exception, BinaryenPop(mod, anyref())),
                        make_return(make_ref_array(stack, std::array{new_boolean(const_boolean(false)), stack.get(exception)})),
                    }),
            };

            auto try_ = BinaryenTry(mod,
                                    nullptr,
                                    make_return(make_ref_array(stack, std::array{new_boolean(const_boolean(true)), call(functions::invoke, std::array{stack.get(f), stack.get(args)})})),
                                    std::data(tags),
                                    std::size(tags),
                                    std::data(catches),
                                    std::size(catches),
                                    nullptr);

            return try_;
        };
    };

    std("pcall", std::array{"f", "..."}, make_pcall(false));

    std("print", std::array{"..."}, [this](function_stack& stack, auto&& vars)
        {
            auto [vaarg] = vars;
            auto i       = stack.alloc(size_type(), "i");

            auto exp = array_get(stack.get(vaarg), stack.get(i), anyref());
            exp      = call(functions::to_string, exp);
            exp      = call(functions::lua_str_to_js_array, exp);
            exp      = make_call("stdout", exp, BinaryenTypeNone());
            exp = BinaryenLoop(mod,
                               "+loop",
                               make_if(binop(BinaryenLtUInt32(), stack.get(i), array_len(stack.get(vaarg))),
                                       make_block(std::array{
                                           exp,
                                           stack.set(i, binop(BinaryenAddInt32(), stack.get(i), const_i32(1))),
                                           BinaryenBreak(mod, "+loop", nullptr, nullptr),
                                       })));

            return std::array{
                exp,
                make_call("stdout", call(functions::lua_str_to_js_array, add_string("\n")), BinaryenTypeNone()),
                make_return(null()),
            };
        });
    std("rawequal", std::array{"v1", "v2"}, [this](function_stack& stack, auto&& vars)
        {
            return BinaryenUnreachable(mod);
        });
    std("rawget", std::array{"table", "index"}, [this](function_stack& stack, auto&& vars)
        {
            return BinaryenUnreachable(mod);
        });
    std("rawlen", std::array{"v"}, [this](function_stack& stack, auto&& vars)
        {
            return BinaryenUnreachable(mod);
        });
    std("rawset", std::array{"table", "index", "value"}, [this](function_stack& stack, auto&& vars)
        {
            return BinaryenUnreachable(mod);
        });
    std("select", std::array{"index", "..."}, [this](function_stack& stack, auto&& vars)
        {
            return BinaryenUnreachable(mod);
        });
    std("setmetatable", std::array{"table", "metatable"}, [this](function_stack& stack, auto&& vars)
        {
            return BinaryenUnreachable(mod);
        });
    std("tonumber", std::array{"e", "base"}, [this](function_stack& stack, auto&& vars)
        {
            auto [e, base] = vars;
            auto num       = call(functions::to_number, stack.get(e));
            return make_return(ref_array::create_fixed(*this, num));
        });
    std("tostring", std::array{"v"}, [this](function_stack& stack, auto&& vars)
        {
            auto [v] = vars;

            auto exp = call(functions::to_string, stack.get(v));
            return make_return(ref_array::create_fixed(*this, exp));
        });
    std("type", std::array{"v"}, [this](function_stack& stack, auto&& vars)
        {
            auto [v] = vars;

            auto casts = std::array{
                value_type::boolean,
                value_type::number,
                value_type::integer,
                value_type::string,
                value_type::function,
                value_type::table,
                value_type::userdata,
                value_type::thread,
            };

            return switch_value(stack.get(v), casts, [&](value_type type, expr_ref exp)
                                {
                                    const char* str;
                                    switch (type)
                                    {
                                    case value_type::nil:
                                        str = "nil";
                                        break;
                                    case value_type::boolean:
                                        str = "boolean";
                                        break;
                                    case value_type::integer:
                                    case value_type::number:
                                        str = "number";
                                        break;
                                    case value_type::string:
                                        str = "string";
                                        break;
                                    case value_type::function:
                                        str = "function";
                                        break;
                                    case value_type::userdata:
                                        str = "userdata";
                                        break;
                                    case value_type::thread:
                                        str = "thread";
                                        break;
                                    case value_type::table:
                                        str = "table";
                                        break;
                                    default:
                                        return BinaryenUnreachable(mod);
                                    }

                                    auto ret = make_return(ref_array::create_fixed(*this, add_string(str)));
                                    return exp ? make_block(std::array{
                                                     drop(exp),
                                                     ret,
                                                 })
                                               : ret;
                                });
        });
    // std("xpcall", std::array{"f", "msgh", "..."}, make_pcall(true));

    std.result.push_back(local_get(0, get_type<table>()));

    return {std::vector<BinaryenType>{}, make_block(std.result)};
}

} // namespace wumbo
