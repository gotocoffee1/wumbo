#include "compiler.hpp"

namespace wumbo
{

expr_ref_list compiler::open_basic_lib()
{
    BinaryenAddFunctionImport(mod, "print_integer", "print", "value", integer_type(), BinaryenTypeNone());
    BinaryenAddFunctionImport(mod, "print_number", "print", "value", number_type(), BinaryenTypeNone());
    BinaryenAddFunctionImport(mod, "print_string", "print", "string", BinaryenTypeExternref(), BinaryenTypeNone());
    BinaryenAddFunctionImport(mod, "print_nil", "print", "value", BinaryenTypeNullref(), BinaryenTypeNone());
    BinaryenAddFunctionImport(mod, "new_u8array", "print", "array", size_type(), BinaryenTypeExternref());

    {
        BinaryenType types[] = {
            BinaryenTypeExternref(),
            size_type(),
            size_type(),
        };
        BinaryenAddFunctionImport(mod, "set_array", "print", "set_array", BinaryenTypeCreate(std::data(types), std::size(types)), BinaryenTypeNone());
    }
    {
        BinaryenType locals[] = {
            size_type(),
            BinaryenTypeExternref(),
        };

        auto str     = local_get(0, type<value_types::string>()); // get string
        auto str_len = array_len(str);                            // get string len
        BinaryenAddFunction(mod,
                            "*lua_str_to_js_array",
                            type<value_types::string>(),
                            BinaryenTypeExternref(),
                            std::data(locals),
                            std::size(locals),
                            make_block(std::array{
                                local_set(1, str_len),
                                local_set(2, make_call("new_u8array", str_len, BinaryenTypeExternref())),
                                make_if(local_get(1, size_type()),
                                        BinaryenLoop(mod,
                                                     "+loop",
                                                     make_block(std::array{
                                                         make_call("set_array",
                                                                   std::array{
                                                                       local_get(2, BinaryenTypeExternref()),
                                                                       local_tee(1, BinaryenBinary(mod, BinaryenSubInt32(), local_get(1, size_type()), const_i32(1)), size_type()),
                                                                       array_get(str, local_get(1, size_type()), char_type()),
                                                                   },
                                                                   BinaryenTypeNone()),
                                                         BinaryenBreak(mod,
                                                                       "+loop",
                                                                       local_get(1, size_type()),
                                                                       nullptr),
                                                     }))),

                                make_return(local_get(2, BinaryenTypeExternref())),
                            }));
    }

    expr_ref_list result;

    auto add_func = [&](const char* name, const name_list& args, bool vararg, auto&& f)
    {
        result.push_back(set_var(name, add_func_ref(name, args, true, f)));
    };

    add_func("load", {"chunk", "chunkname", "mode", "env"}, true, [this]()
             {
                 auto exp = get_var("f");
                 return std::array{throw_error(get_var("message"))};
             });
    add_func("pcall", {"f", "arg1"}, true, [this]()
             {
                 auto exp = get_var("f");
                 return std::array{throw_error(get_var("message"))};
             });

    add_func("assert", {"v", "message"}, false, [this]()
             {
                 return std::array{BinaryenUnreachable(mod)};
             });

    add_func("tostring ", {"v"}, false, [this]()
             {
                 return std::array{BinaryenUnreachable(mod)};
             });
    add_func("error", {"message", "level"}, false, [this]()
             {
                 return std::array{throw_error(get_var("message"))};
             });
    add_func("print", {}, true, [this]()
             {
                 auto exp = (*this)(ellipsis{});

                 exp = array_get(exp, const_i32(0), anyref());

                 auto casts = std::array{
                     //    value_types::boolean,
                     value_types::number,
                     value_types::integer,
                     value_types::string,
                 };

                 auto s = [&](value_types type, expr_ref exp)
                 {
                     const char* func;
                     switch (type)
                     {
                     case value_types::nil:
                         exp  = null();
                         func = "print_nil";
                         break;
                     case value_types::boolean:
                         exp  = null();
                         func = "print_nil";
                         break;
                     case value_types::integer:
                         func = "print_integer";
                         exp  = BinaryenStructGet(mod, 0, exp, integer_type(), false);
                         break;
                     case value_types::number:
                         func = "print_number";
                         exp  = BinaryenStructGet(mod, 0, exp, number_type(), false);
                         break;
                     case value_types::string:
                     {
                         func = "print_string";
                         exp  = make_call("*lua_str_to_js_array", exp, BinaryenTypeExternref());
                         break;
                     }
                     case value_types::function:
                     case value_types::userdata:
                     case value_types::thread:
                     case value_types::table:
                     case value_types::dynamic:
                     default:
                         return BinaryenUnreachable(mod);
                     }
                     return make_block(std::array{
                         make_call(func, exp, BinaryenTypeNone()),
                         make_return(null()),
                     });
                 };

                 return switch_value(exp, casts, s);
             });

    return result;
}

expr_ref_list compiler::setup_env()
{
    local_variables vars;
    vars.names.push_back("_ENV");
    vars.explist.emplace_back().inner = table_constructor{};

    return (*this)(vars);

    // todo:
    //local _ENV = {}
    //_ENV._G = _ENV
}

} // namespace wumbo
