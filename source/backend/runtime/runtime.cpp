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

runtime::function_stack::func_t runtime::compare(value_type vtype)
{
    runtime::function_stack stack{mod};

    auto cmp = [&](runtime::function_stack& stack)
    {
        auto first  = stack.alloc(type(vtype), "first");
        auto second = stack.alloc(anyref(), "second");
        stack.locals();

        auto casts = std::array{
            vtype,
        };
        return make_block(
            switch_value(stack.get(second),
                         casts,
                         [&](value_type type_right, expr_ref exp_right)
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
                                 return make_return(binop(BinaryenEqInt64(), integer::get<integer::inner>(*this, stack.get(first)), integer::get<integer::inner>(*this, exp_right)));
                             case value_type::number:
                                 return make_return(binop(BinaryenEqFloat64(), number::get<number::inner>(*this, stack.get(first)), number::get<number::inner>(*this, exp_right)));
                             case value_type::table:
                             case value_type::function:
                             case value_type::thread:
                                 return BinaryenRefEq(mod, stack.get(first), exp_right);

                             case value_type::string:
                             {
                                 auto string_t = type<value_type::string>();

                                 auto i           = stack.alloc(size_type(), "i");
                                 auto exp_i       = stack.alloc(string_t, "exp_i");
                                 auto exp_right_i = stack.alloc(string_t, "exp_right_i");

                                 auto dec = [&](expr_ref first)
                                 {
                                     return binop(BinaryenSubInt32(),
                                                  first,
                                                  const_i32(1));
                                 };

                                 return make_return(
                                     make_if(binop(BinaryenEqInt32(), stack.tee(i, array_len(stack.tee(exp_i, stack.get(0)))), array_len(stack.tee(exp_right_i, exp_right))),
                                             BinaryenLoop(mod,
                                                          "+loop",
                                                          BinaryenIf(mod,
                                                                     stack.get(i),
                                                                     make_block(std::array{
                                                                         BinaryenBreak(mod,
                                                                                       "+loop",
                                                                                       binop(BinaryenEqInt32(),
                                                                                             string::get(*this, stack.get(exp_i), stack.tee(i, dec(stack.get(i)))),
                                                                                             string::get(*this, stack.get(exp_right_i), stack.get(i))),
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
    };
    return stack.add_function(("*key_compare_"s + type_name(vtype)).c_str(), size_type(), cmp);
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
        value_type::boolean,
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
                                        case value_type::boolean:
                                            exp = make_if(BinaryenI31Get(mod, exp, false), add_string("true"), add_string("false"));
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

build_return_t runtime::get_type_num()
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

build_return_t runtime::box_integer()
{
    return {std::vector<BinaryenType>{}, new_integer(local_get(0, integer_type()))};
}

build_return_t runtime::box_number()
{
    return {std::vector<BinaryenType>{}, new_number(local_get(0, number_type()))};
}

build_return_t runtime::to_js_integer()
{
    return {std::vector<BinaryenType>{}, unbox_integer(BinaryenRefCast(mod, local_get(0, anyref()), type<value_type::integer>()))};
}

build_return_t runtime::to_js_string()
{
    return {std::vector<BinaryenType>{}, call(functions::lua_str_to_js_array, BinaryenRefCast(mod, local_get(0, anyref()), type<value_type::string>()))};
}

build_return_t runtime::any_array_size()
{
    return {std::vector<BinaryenType>{}, array_len(local_get(0, ref_array_type()))};
}

build_return_t runtime::any_array_get()
{
    return {std::vector<BinaryenType>{}, array_get(local_get(0, ref_array_type()), local_get(1, size_type()), anyref())};
}

build_return_t runtime::any_array_set()
{
    return {std::vector<BinaryenType>{}, array_set(local_get(0, ref_array_type()), local_get(1, size_type()), local_get(2, anyref()))};
}

build_return_t runtime::any_array_create()
{
    return {std::vector<BinaryenType>{}, BinaryenArrayNew(mod, BinaryenTypeGetHeapType(ref_array_type()), local_get(0, size_type()), nullptr)};
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
