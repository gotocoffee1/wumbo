#include "ast/ast.hpp"
#include "backend/wasm_util.hpp"
#include "binaryen-c.h"
#include "compiler.hpp"
#include "runtime/runtime.hpp"
#include <array>

namespace wumbo
{

expr_ref_list compiler::open_basic_lib()
{
    BinaryenAddFunctionImport(mod, "stdout", "native", "stdout", BinaryenTypeExternref(), BinaryenTypeNone());
    BinaryenAddFunctionImport(mod, "load_lua", "load", "load", BinaryenTypeExternref(), lua_func());

    expr_ref_list result;

    auto add_func = [&](const char* name, const name_list& args, bool vararg, auto&& f)
    {
        std::vector<local_usage> usage;
        usage.resize(args.size());
        result.push_back(set_var(name, add_func_ref(name, args, usage, vararg, f)));
    };

    add_func("assert", {"v" /*, "message"*/}, false, [this]()
             {
                 auto alt = [&]()
                 {
                     return add_string("assertion failed!");
                 };
                 return std::array{make_if(_runtime.call(functions::to_bool, get_var("v")), make_return(local_get(args_index, ref_array_type())), throw_error(at_or_null(args_index, 1, nullptr, alt)))};
             });
    add_func("collectgarbage", {"opt", "arg"}, false, [this]()
             {
                 return std::array{BinaryenUnreachable(mod)};
             });
    add_func("dofile", {"filename"}, false, [this]()
             {
                 return std::array{BinaryenUnreachable(mod)};
             });
    add_func("error", {"message", "level"}, false, [this]()
             {
                 return std::array{throw_error(get_var("message"))};
             });
    add_func("getmetatable", {"object"}, false, [this]()
             {
                 return std::array{BinaryenUnreachable(mod)};
             });
    add_func("ipairs", {"t"}, false, [this]()
             {
                 return std::array{BinaryenUnreachable(mod)};
             });
    add_func("load", {"chunk", "chunkname", "mode", "env"}, false, [&]()
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
             });
    add_func("loadfile", {"filename ", "mode", "env"}, false, [this]()
             {
                 return std::array{BinaryenUnreachable(mod)};
             });
    add_func("next", {"table", "index"}, false, [this]()
             {
                 return std::array{BinaryenUnreachable(mod)};
             });
    add_func("pairs", {"t"}, false, [this]()
             {
                 return std::array{BinaryenUnreachable(mod)};
             });

    auto make_pcall = [this](bool x)
    {
        return [this, x]()
        {
            auto f = get_var("f");

            auto args = (*this)(ellipsis{});

            auto exception = help_var_scope{_func_stack, anyref()};

            const char* tags[] = {error_tag};
            expr_ref catches[] = {
                x ? make_block(std::array{
                        local_set(exception, BinaryenPop(mod, anyref())),
                        drop(call(get_var("msgh"), make_ref_array(local_get(exception, anyref())))),
                        make_return(make_ref_array({new_boolean(const_boolean(false)), local_get(exception, anyref())})),
                    })
                  : make_block(std::array{
                        local_set(exception, BinaryenPop(mod, anyref())),
                        make_return(make_ref_array({new_boolean(const_boolean(false)), local_get(exception, anyref())})),
                    }),
            };

            auto try_ = BinaryenTry(mod,
                                    nullptr,
                                    make_return(make_ref_array({new_boolean(const_boolean(true)), call(f, args)})),
                                    std::data(tags),
                                    std::size(tags),
                                    std::data(catches),
                                    std::size(catches),
                                    nullptr);

            return std::array{try_};
        };
    };

    add_func("pcall", {"f"}, true, make_pcall(false));
    add_func("print", {}, true, [this]()
             {
                 auto exp = (*this)(ellipsis{});

                 exp = array_get(exp, const_i32(0), anyref());
                 exp = _runtime.call(functions::to_string, exp);
                 exp = _runtime.call(functions::lua_str_to_js_array, exp);

                 return std::array{
                     make_call("stdout", exp, BinaryenTypeNone()),
                     make_call("stdout", _runtime.call(functions::lua_str_to_js_array, add_string("\n")), BinaryenTypeNone()),
                     make_return(null()),
                 };
             });
    add_func("rawequal", {"v1", "v2"}, false, [this]()
             {
                 return std::array{BinaryenUnreachable(mod)};
             });
    add_func("rawget", {"table", "index"}, false, [this]()
             {
                 return std::array{BinaryenUnreachable(mod)};
             });
    add_func("rawlen", {"v"}, false, [this]()
             {
                 return std::array{BinaryenUnreachable(mod)};
             });
    add_func("rawset", {"table", "index", "value"}, false, [this]()
             {
                 return std::array{BinaryenUnreachable(mod)};
             });
    add_func("select", {"index"}, true, [this]()
             {
                 return std::array{BinaryenUnreachable(mod)};
             });
    add_func("setmetatable", {"table", "metatable"}, false, [this]()
             {
                 return std::array{BinaryenUnreachable(mod)};
             });
    add_func("tonumber", {"e", "base"}, false, [this]()
             {
                 auto e = get_var("e");
                 e      = _runtime.call(functions::to_number, e);
                 return std::array{make_return(make_ref_array(e))};
             });
    add_func("tostring", {"v"}, false, [this]()
             {
                 auto exp = _runtime.call(functions::to_string, get_var("v"));
                 return std::array{make_return(make_ref_array(exp))};
             });
    add_func("type", {"v"}, false, [this]()
             {
                 auto v = get_var("v");

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

                 return switch_value(v, casts, [&](value_type type, expr_ref exp)
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

                                         auto ret = make_return(make_ref_array(add_string(str)));
                                         return exp ? make_block(std::array{
                                                          drop(exp),
                                                          ret,
                                                      })
                                                    : ret;
                                     });
             });
    add_func("xpcall", {"f", "msgh"}, true, make_pcall(true));
    return result;
}

expr_ref_list compiler::setup_env()
{
    local_variables vars;
    vars.names.push_back("_ENV");
    vars.explist.emplace_back().inner = table_constructor{};
    auto& usage                       = vars.usage.emplace_back();
    usage.upvalue                     = true;
    usage.read_count                  = 1;
    usage.write_count                 = 0;

    return (*this)(vars);

    // todo:
    //local _ENV = {}
    //_ENV._G = _ENV
}

} // namespace wumbo
