#include "runtime.hpp"

#include "backend/wasm_util.hpp"
#include "binaryen-c.h"
#include "utils/type.hpp"

#include <array>
#include <cstdint>
#include <functional>

namespace wumbo
{

const func_sig& runtime::require(functions function)
{
    size_t index = static_cast<std::underlying_type_t<functions>>(function);
    if (index >= _required_functions.size())
        _required_functions.resize(index + 1);
    _required_functions[index] = true;
    return _funcs[index];
}

void runtime::build_types()
{
    ext_types::build_types();
#define FUNC_LIST(name, params, ret) {#name, params, ret, &runtime::name},
    _funcs = decltype(_funcs){{RUNTIME_FUNCTIONS(FUNC_LIST)}};
#undef FUNC_LIST
}

void runtime::build()
{
    for (size_t i = 0; i < std::size(_funcs); ++i)
    {
        auto& f = _funcs[i];
        bool use;
        if (i >= _required_functions.size())
            use = false;
        else
            use = _required_functions[i];

        auto test = [use](auto action)
        {
            switch (action)
            {
            case function_action::required:
                if (!use)
                    break;
                [[fallthrough]];
            case function_action::all:
                return true;
            default:
                break;
            }
            return false;
        };
        if (test(import_functions))
        {
            import_func(f.name, f.params_type, f.return_type, "runtime");
        }
        if (test(export_functions))
        {
            export_func(f.name);
        }
        if (test(create_functions))
        {
            auto [locals, body] = std::invoke(f.build, this);
            BinaryenAddFunction(mod,
                                f.name,
                                f.params_type,
                                f.return_type,
                                std::data(locals),
                                std::size(locals),
                                body);
        }
    }
}

BinaryenFunctionRef runtime::compare(const char* name, value_type vtype)
{
    auto casts = std::array{
        vtype,
    };
    auto exp = local_get(0, type(vtype));

    BinaryenType params[] = {
        type(vtype),
        anyref(),
    };
    std::vector<BinaryenType> vars;

    auto block = make_block(switch_value(local_get(1, anyref()), casts, [&](value_type type_right, expr_ref exp_right)
                                         {
                                             if (vtype != type_right)
                                             {
                                                 if (!exp_right)
                                                     return BinaryenUnreachable(mod);
                                                 return const_i32(0);
                                             }
                                             switch (type_right)
                                             {
                                             case value_type::integer:
                                                 return make_return(BinaryenBinary(mod, BinaryenEqInt64(), BinaryenStructGet(mod, 0, exp, integer_type(), false), BinaryenStructGet(mod, 0, exp_right, integer_type(), false)));
                                             case value_type::number:
                                                 return make_return(BinaryenBinary(mod, BinaryenEqFloat64(), BinaryenStructGet(mod, 0, exp, number_type(), false), BinaryenStructGet(mod, 0, exp_right, number_type(), false)));
                                             case value_type::table:
                                             case value_type::function:
                                             case value_type::thread:
                                                 return BinaryenRefEq(mod, exp, exp_right);

                                             case value_type::string:
                                             {
                                                 auto string_t = type<value_type::string>();
                                                 auto i        = vars.size() + std::size(params);
                                                 vars.push_back(size_type());
                                                 auto exp_i = vars.size() + std::size(params);
                                                 vars.push_back(string_t);
                                                 auto exp_right_i = vars.size() + std::size(params);
                                                 vars.push_back(string_t);

                                                 auto dec = [&](expr_ref first)
                                                 {
                                                     return BinaryenBinary(mod,
                                                                           BinaryenSubInt32(),
                                                                           first,
                                                                           const_i32(1));
                                                 };

                                                 return make_return(
                                                     make_if(
                                                         BinaryenBinary(mod, BinaryenEqInt32(), local_tee(i, array_len(local_tee(exp_i, exp, string_t)), size_type()), array_len(local_tee(exp_right_i, exp_right, string_t))),
                                                         BinaryenLoop(mod,
                                                                      "+loop",
                                                                      BinaryenIf(mod,
                                                                                 local_get(i, size_type()),
                                                                                 make_block(std::array{
                                                                                     BinaryenBreak(mod,
                                                                                                   "+loop",
                                                                                                   BinaryenBinary(mod,
                                                                                                                  BinaryenEqInt32(),
                                                                                                                  array_get(local_get(exp_i, string_t), local_tee(i, dec(local_get(i, size_type())), size_type()), char_type()),
                                                                                                                  array_get(local_get(exp_right_i, string_t), local_get(i, size_type()), char_type())),
                                                                                                   nullptr),
                                                                                     const_i32(0),
                                                                                 }),
                                                                                 const_i32(1))),

                                                         const_i32(0)));
                                             }
                                             case value_type::nil:
                                             case value_type::boolean:
                                             case value_type::userdata:
                                             default:
                                                 return BinaryenUnreachable(mod);
                                             }
                                         }),
                            nullptr,
                            size_type());

    return BinaryenAddFunction(mod,
                               (std::string("*key_compare_") + name).c_str(),
                               BinaryenTypeCreate(std::data(params), std::size(params)),
                               size_type(),
                               std::data(vars),
                               std::size(vars),
                               block);
}

build_return_t runtime::table_set()
{
    auto init_table_type_set = [this](const std::string& name, value_type vtype)
    {
        auto table = local_get(0, type<value_type::table>());
        auto key   = local_get(1, type(vtype));
        auto value = local_get(2, anyref());

        auto bucket = BinaryenStructGet(mod,
                                        tbl_hash_index,
                                        table,
                                        ref_array_type(),
                                        false);

        BinaryenType params[] = {
            type<value_type::table>(),
            type(vtype),
            anyref(),
        };

        BinaryenType locals[] = {
            size_type(),
            ref_array_type(),
        };

        size_t i         = std::size(params);
        size_t new_array = std::size(params) + 1;

        expr_ref args[] = {
            key,
            array_get(bucket, local_tee(i, BinaryenBinary(mod, BinaryenSubInt32(), local_get(i, size_type()), const_i32(2)), size_type()), anyref()),
        };

        return BinaryenAddFunction(mod,
                                   ("*table_set_"s + name).c_str(),
                                   BinaryenTypeCreate(std::data(params), std::size(params)),
                                   BinaryenTypeNone(),
                                   std::data(locals),
                                   std::size(locals),
                                   make_block(std::array{
                                       local_set(i, array_len(bucket)),
                                       BinaryenLoop(mod,
                                                    "+loop",
                                                    make_if(local_get(i, size_type()),
                                                            make_block(std::array{
                                                                BinaryenBreak(mod,
                                                                              "+loop",
                                                                              BinaryenUnary(mod, BinaryenEqZInt32(), BinaryenCall(mod, ("*key_compare_" + name).c_str(), std::data(args), std::size(args), size_type())),
                                                                              nullptr),
                                                                BinaryenArraySet(mod,
                                                                                 bucket,
                                                                                 BinaryenBinary(mod, BinaryenAddInt32(), local_get(i, size_type()), const_i32(1)),
                                                                                 value),
                                                                make_return(),
                                                            }))),
                                       resize_array(new_array, ref_array_type(), bucket, const_i32(2), true),

                                       BinaryenStructSet(mod, tbl_hash_index, table, local_get(new_array, ref_array_type())),
                                       BinaryenArraySet(mod, local_get(new_array, ref_array_type()), const_i32(0), key),
                                       BinaryenArraySet(mod, local_get(new_array, ref_array_type()), const_i32(1), value),
                                   }));
    };
    auto table = local_get(0, anyref());
    auto key   = local_get(1, anyref());
    auto value = local_get(2, anyref());
    table      = BinaryenRefCast(mod, table, type<value_type::table>());

    //auto hash = calc_hash(key);

    //auto bucket = find_bucket(table, hash);

    //auto bucket = ;

    auto casts = std::array{
        value_type::number,
        value_type::integer,
        value_type::string,
    };

    //for (auto value : casts)
    //    compare(type_name(value), value);

    for (auto value : casts)
        init_table_type_set(type_name(value), value);

    return {std::vector<BinaryenType>{}, make_block(switch_value(key, casts, [&](value_type type, expr_ref exp)
                                                                 {
                                                                     expr_ref args[] = {
                                                                         table,
                                                                         exp,
                                                                         value,
                                                                     };

                                                                     const char* func;
                                                                     switch (type)
                                                                     {
                                                                     case value_type::integer:
                                                                         func = "*table_set_integer";
                                                                         break;
                                                                     case value_type::number:
                                                                         func = "*table_set_number";
                                                                         break;
                                                                     case value_type::string:
                                                                         func = "*table_set_string";
                                                                         break;
                                                                     case value_type::nil:
                                                                     case value_type::boolean:
                                                                     case value_type::function:
                                                                     case value_type::userdata:
                                                                     case value_type::thread:
                                                                     case value_type::table:

                                                                     default:
                                                                         return BinaryenUnreachable(mod);
                                                                     }

                                                                     return BinaryenReturnCall(mod,
                                                                                               func,
                                                                                               std::data(args),
                                                                                               std::size(args),
                                                                                               BinaryenTypeNone());
                                                                 }))};
}

build_return_t runtime::table_get()
{
    auto init_table_type_get = [this](const std::string& name, value_type vtype)
    {
        auto table = local_get(0, type<value_type::table>());
        auto key   = local_get(1, type(vtype));

        auto bucket = BinaryenStructGet(mod,
                                        tbl_hash_index,
                                        table,
                                        ref_array_type(),
                                        false);

        BinaryenType params[] = {
            type<value_type::table>(),
            type(vtype),
        };

        BinaryenType locals[] = {
            size_type(),
        };

        size_t i = std::size(params);

        return BinaryenAddFunction(mod,
                                   ("*table_get_" + name).c_str(),
                                   BinaryenTypeCreate(std::data(params), std::size(params)),
                                   anyref(),
                                   std::data(locals),
                                   std::size(locals),
                                   make_block(std::array{
                                       local_set(i, array_len(bucket)),
                                       BinaryenLoop(mod,
                                                    "+loop",
                                                    make_if(
                                                        local_get(i, size_type()),
                                                        make_block(std::array{
                                                            BinaryenBreak(mod,
                                                                          "+loop",
                                                                          BinaryenUnary(mod,
                                                                                        BinaryenEqZInt32(),
                                                                                        make_call(("*key_compare_" + name).c_str(), std::array{
                                                                                                                                        key,
                                                                                                                                        array_get(bucket, local_tee(i, BinaryenBinary(mod, BinaryenSubInt32(), local_get(i, size_type()), const_i32(2)), size_type()), anyref()),
                                                                                                                                    },
                                                                                                  size_type())),
                                                                          nullptr),
                                                            make_return(BinaryenArrayGet(mod, bucket, BinaryenBinary(mod, BinaryenAddInt32(), local_get(i, size_type()), const_i32(1)), anyref(), false)),
                                                        }))),

                                       make_return(null()),
                                   }));
    };

    auto table = local_get(0, anyref());
    auto key   = local_get(1, anyref());
    table      = BinaryenRefCast(mod, table, type<value_type::table>());

    //auto hash = calc_hash(key);

    //auto bucket = find_bucket(table, hash);

    //auto bucket = ;

    auto casts = std::array{
        value_type::number,
        value_type::integer,
        value_type::string,
    };

    for (auto value : casts)
        compare(type_name(value), value);

    for (auto value : casts)
        init_table_type_get(type_name(value), value);

    return {std::vector<BinaryenType>{},
            make_block(switch_value(key,
                                    casts,
                                    [&](value_type type, expr_ref exp)
                                    {
                                        expr_ref args[] = {
                                            table,
                                            exp,
                                        };

                                        const char* func;
                                        switch (type)
                                        {
                                        case value_type::integer:
                                            func = "*table_get_integer";
                                            break;
                                        case value_type::number:
                                            func = "*table_get_number";
                                            break;
                                        case value_type::string:
                                            func = "*table_get_string";
                                            break;
                                        case value_type::nil:
                                        case value_type::boolean:
                                        case value_type::function:
                                        case value_type::userdata:
                                        case value_type::thread:
                                        case value_type::table:

                                        default:
                                            return BinaryenUnreachable(mod);
                                        }

                                        return BinaryenReturnCall(mod,
                                                                  func,
                                                                  std::data(args),
                                                                  std::size(args),
                                                                  anyref());
                                    }))};
}

build_return_t runtime::to_bool()
{
    auto casts = std::array{
        value_type::boolean,
    };
    return {std::vector<BinaryenType>{},
            make_block(switch_value(local_get(0, anyref()), casts, [&](value_type type, expr_ref exp)
                                    {
                                        switch (type)
                                        {
                                        case value_type::nil:
                                            return make_return(const_i32(0));
                                        case value_type::boolean:
                                            return make_return(BinaryenI31Get(mod, exp, false));
                                        default:
                                            return make_return(const_i32(1));
                                        }
                                    }))};
}

build_return_t runtime::to_bool_not()
{
    auto casts = std::array{
        value_type::boolean,
    };

    return {std::vector<BinaryenType>{},
            make_block(switch_value(local_get(0, anyref()), casts, [&](value_type type, expr_ref exp)
                                    {
                                        switch (type)
                                        {
                                        case value_type::nil:
                                            return make_return(const_i32(1));
                                        case value_type::boolean:
                                            return make_return(BinaryenUnary(mod, BinaryenEqZInt32(), BinaryenI31Get(mod, exp, false)));
                                        default:
                                            return make_return(const_i32(0));
                                        }
                                    }))};
}

build_return_t runtime::to_string()
{
    import_func("int_to_str", integer_type(), BinaryenTypeExternref(), "native", "toString");
    import_func("num_to_str", number_type(), BinaryenTypeExternref(), "native", "toString");
    auto casts = std::array{
        value_type::string,
        value_type::number,
        value_type::integer,
    };
    return {std::vector<BinaryenType>{},
            make_block(switch_value(local_get(0, anyref()), casts, [&](value_type type, expr_ref exp)
                                    {
                                        switch (type)
                                        {
                                        case value_type::nil:
                                            exp = add_string("nil");
                                            break;
                                        case value_type::string:
                                            break;
                                        case value_type::integer:
                                            exp = BinaryenStructGet(mod, 0, exp, integer_type(), false);
                                            exp = make_call("int_to_str", exp, BinaryenTypeExternref());
                                            exp = call(functions::js_array_to_lua_str, exp);
                                            break;
                                        case value_type::number:
                                            exp = BinaryenStructGet(mod, 0, exp, integer_type(), false);
                                            exp = make_call("num_to_str", exp, BinaryenTypeExternref());
                                            exp = call(functions::js_array_to_lua_str, exp);
                                            break;
                                        default:
                                            exp = null();
                                            break;
                                        }
                                        return make_return(exp);
                                    }))};
}

build_return_t runtime::to_number()
{
    import_func("str_to_int", BinaryenTypeExternref(), integer_type(), "native", "toInt");
    import_func("str_to_num", BinaryenTypeExternref(), number_type(), "native", "toNum");

    auto casts = std::array{
        value_type::string,
        value_type::number,
        value_type::integer,
    };
    return {std::vector<BinaryenType>{},
            make_block(switch_value(local_get(0, anyref()), casts, [&](value_type type, expr_ref exp)
                                    {
                                        switch (type)
                                        {
                                        case value_type::string:
                                            exp = call(functions::lua_str_to_js_array, exp);
                                            exp = make_call("str_to_num", exp, number_type());
                                            exp = new_number(exp);
                                            break;
                                        case value_type::integer:
                                        case value_type::number:
                                            break;
                                        default:
                                            exp = null();
                                            break;
                                        }
                                        return make_return(exp);
                                    }))};
}

build_return_t runtime::lua_str_to_js_array()
{
    import_func("buffer_new", size_type(), BinaryenTypeExternref(), "buffer", "new");
    import_func("buffer_set", create_type(BinaryenTypeExternref(), size_type(), char_type()), BinaryenTypeNone(), "buffer", "set");

    auto str     = local_get(0, type<value_type::string>()); // get string
    auto str_len = array_len(str);                           // get string len

    return {std::vector<BinaryenType>{
                size_type(),
                BinaryenTypeExternref(),
            },
            make_block(std::array{
                local_set(2, make_call("buffer_new", local_tee(1, str_len, size_type()), BinaryenTypeExternref())),
                make_if(local_get(1, size_type()),
                        BinaryenLoop(mod,
                                     "+loop",
                                     make_block(std::array{
                                         make_call("buffer_set",
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
            })};
}

build_return_t runtime::js_array_to_lua_str()
{
    import_func("buffer_size", BinaryenTypeExternref(), size_type(), "buffer", "size");
    import_func("buffer_get", create_type(BinaryenTypeExternref(), size_type()), char_type(), "buffer", "get");

    auto str     = local_get(0, BinaryenTypeExternref());      // get string
    auto str_len = make_call("buffer_size", str, size_type()); // get string len

    return {std::vector<BinaryenType>{
                size_type(),
                type<value_type::string>(),
            },
            make_block(std::array{
                local_set(2, BinaryenArrayNew(mod, BinaryenTypeGetHeapType(type<value_type::string>()), local_tee(1, str_len, size_type()), nullptr)),
                make_if(local_get(1, size_type()),
                        BinaryenLoop(mod,
                                     "+loop",
                                     make_block(std::array{
                                         array_set(local_get(2, type<value_type::string>()),
                                                   local_tee(1, BinaryenBinary(mod, BinaryenSubInt32(), local_get(1, size_type()), const_i32(1)), size_type()),
                                                   make_call("buffer_get",
                                                             std::array{
                                                                 str,
                                                                 local_get(1, size_type()),
                                                             },
                                                             char_type())),
                                         BinaryenBreak(mod,
                                                       "+loop",
                                                       local_get(1, size_type()),
                                                       nullptr),
                                     }))),

                make_return(local_get(2, type<value_type::string>())),
            })};
}

build_return_t runtime::get_type()
{
    auto v = local_get(0, anyref());

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

    return {std::vector<BinaryenType>{},
            make_block(switch_value(v, casts, [&](value_type type, expr_ref exp)
                                    {
                                        int32_t num;
                                        switch (type)
                                        {
                                        case value_type::nil:
                                        case value_type::boolean:
                                        case value_type::integer:
                                        case value_type::number:
                                        case value_type::string:
                                        case value_type::function:
                                        case value_type::userdata:
                                        case value_type::thread:
                                        case value_type::table:
                                            num = static_cast<int32_t>(type);
                                            break;
                                        default:
                                            return BinaryenUnreachable(mod);
                                        }

                                        auto ret = make_return(const_i32(num));
                                        return exp ? make_block(std::array{
                                                         drop(exp),
                                                         ret,
                                                     })
                                                   : ret;
                                    }))};
}

build_return_t runtime::to_js_int()
{
    return {std::vector<BinaryenType>{}, unbox_integer(BinaryenRefCast(mod, local_get(0, anyref()), type<value_type::integer>()))};
}

build_return_t runtime::to_js_string()
{
    return {std::vector<BinaryenType>{}, call(functions::lua_str_to_js_array, BinaryenRefCast(mod, local_get(0, anyref()), type<value_type::string>()))};
}

build_return_t runtime::invoke()
{
    auto casts = std::array{
        value_type::function,
    };
    return {std::vector<BinaryenType>{type<value_type::function>()},
            make_block(switch_value(local_get(0, anyref()), casts, [&](value_type exp_type, expr_ref exp)
                                    {
                                        switch (exp_type)
                                        {
                                        case value_type::function:
                                        {
                                            auto t     = type<value_type::function>();
                                            auto local = 2;

                                            auto func_ref = BinaryenStructGet(mod, 0, local_get(local, t), BinaryenTypeFuncref(), false);
                                            auto upvalues = BinaryenStructGet(mod, 1, local_tee(local, exp, t), ref_array_type(), false);

                                            expr_ref real_args[2];

                                            real_args[upvalue_index] = upvalues;
                                            real_args[args_index]    = local_get(1, ref_array_type());
                                            return BinaryenCallRef(mod, func_ref, std::data(real_args), std::size(real_args), BinaryenTypeNone(), true);
                                        }

                                        default:
                                            return throw_error(add_string("not a function"));
                                        }
                                    }))};
}
} // namespace wumbo
