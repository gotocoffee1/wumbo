#include "compiler.hpp"

namespace wumbo
{
expr_ref compiler::calc_hash(expr_ref key)
{
    return const_i32(3);
}

expr_ref compiler::find_bucket(expr_ref table, expr_ref hash)
{
    auto hash_map = BinaryenStructGet(mod, tbl_hash_index, table, ref_array_type(), false);

    auto bucket_index = BinaryenBinary(mod, BinaryenRemUInt32(), hash, array_len(hash_map));
    auto bucket       = BinaryenArrayGet(mod, hash_map, bucket_index, ref_array_type(), false);

    return bucket;
}

expr_ref compiler::table_get(expr_ref table, expr_ref key)
{
    return make_call("*table_get",
                     std::array{
                         table,
                         key,
                     },
                     anyref());
}

expr_ref compiler::table_set(expr_ref table, expr_ref key, expr_ref value)
{
    return make_call("*table_set", std::array{
                                       table,
                                       key,
                                       value,
                                   },
                     BinaryenTypeNone());
}

BinaryenFunctionRef compiler::compare(const char* name, value_types vtype)
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

    auto block = make_block(switch_value(local_get(1, anyref()), casts, [&](value_types type_right, expr_ref exp_right)
                                         {
                                             if (vtype != type_right)
                                             {
                                                 if (!exp_right)
                                                     return BinaryenUnreachable(mod);
                                                 return const_i32(0);
                                             }
                                             switch (type_right)
                                             {
                                             case value_types::integer:
                                                 return make_return(BinaryenBinary(mod, BinaryenEqInt64(), BinaryenStructGet(mod, 0, exp, integer_type(), false), BinaryenStructGet(mod, 0, exp_right, integer_type(), false)));
                                             case value_types::number:
                                                 return make_return(BinaryenBinary(mod, BinaryenEqFloat64(), BinaryenStructGet(mod, 0, exp, number_type(), false), BinaryenStructGet(mod, 0, exp_right, number_type(), false)));
                                             case value_types::table:
                                             case value_types::function:
                                             case value_types::thread:
                                                 return BinaryenRefEq(mod, exp, exp_right);

                                             case value_types::string:
                                             {
                                                 auto string_t = type<value_types::string>();
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
                                             case value_types::nil:
                                             case value_types::boolean:
                                             case value_types::userdata:
                                             case value_types::dynamic:
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

void compiler::func_table_get()
{
    auto init_table_type_get = [this](const std::string& name, value_types vtype)
    {
        auto table = local_get(0, type<value_types::table>());
        auto key   = local_get(1, type(vtype));

        auto bucket = BinaryenStructGet(mod,
                                        tbl_hash_index,
                                        table,
                                        ref_array_type(),
                                        false);

        BinaryenType params[] = {
            type<value_types::table>(),
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

    auto init_table_type_set = [this](const std::string& name, value_types vtype)
    {
        auto table = local_get(0, type<value_types::table>());
        auto key   = local_get(1, type(vtype));
        auto value = local_get(2, anyref());

        auto bucket = BinaryenStructGet(mod,
                                        tbl_hash_index,
                                        table,
                                        ref_array_type(),
                                        false);

        BinaryenType params[] = {
            type<value_types::table>(),
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
                                   (std::string("*table_set_") + name).c_str(),
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
    table      = BinaryenRefCast(mod, table, type<value_types::table>());

    //auto hash = calc_hash(key);

    //auto bucket = find_bucket(table, hash);

    //auto bucket = ;

    auto casts = std::array{
        value_types::number,
        value_types::integer,
        value_types::string,
    };

    for (auto value : casts)
        compare(to_string(value), value);

    for (auto value : casts)
        init_table_type_get(to_string(value), value);

    {
        BinaryenType params[] = {
            anyref(),
            anyref(),
        };

        BinaryenAddFunction(mod,
                            "*table_get",
                            BinaryenTypeCreate(std::data(params), std::size(params)),
                            anyref(),
                            nullptr,
                            0,
                            make_block(switch_value(key,
                                                    casts,
                                                    [&](value_types type, expr_ref exp)
                                                    {
                                                        expr_ref args[] = {
                                                            table,
                                                            exp,
                                                        };

                                                        const char* func;
                                                        switch (type)
                                                        {
                                                        case value_types::integer:
                                                            func = "*table_get_integer";
                                                            break;
                                                        case value_types::number:
                                                            func = "*table_get_number";
                                                            break;
                                                        case value_types::string:
                                                            func = "*table_get_string";
                                                            break;
                                                        case value_types::nil:
                                                        case value_types::boolean:
                                                        case value_types::function:
                                                        case value_types::userdata:
                                                        case value_types::thread:
                                                        case value_types::table:

                                                        default:
                                                        case value_types::dynamic:
                                                            return BinaryenUnreachable(mod);
                                                        }

                                                        return BinaryenReturnCall(mod,
                                                                                  func,
                                                                                  std::data(args),
                                                                                  std::size(args),
                                                                                  anyref());
                                                    })));
    }

    for (auto value : casts)
        init_table_type_set(to_string(value), value);

    {
        BinaryenType params[] = {
            anyref(),
            anyref(),
            anyref(),
        };

        BinaryenAddFunction(mod,
                            "*table_set",
                            BinaryenTypeCreate(std::data(params), std::size(params)),
                            BinaryenTypeNone(),
                            nullptr,
                            0,
                            make_block(switch_value(key,
                                                    casts,
                                                    [&](value_types type, expr_ref exp)
                                                    {
                                                        expr_ref args[] = {
                                                            table,
                                                            exp,
                                                            value,
                                                        };

                                                        const char* func;
                                                        switch (type)
                                                        {
                                                        case value_types::integer:
                                                            func = "*table_set_integer";
                                                            break;
                                                        case value_types::number:
                                                            func = "*table_set_number";
                                                            break;
                                                        case value_types::string:
                                                            func = "*table_set_string";
                                                            break;
                                                        case value_types::nil:
                                                        case value_types::boolean:
                                                        case value_types::function:
                                                        case value_types::userdata:
                                                        case value_types::thread:
                                                        case value_types::table:

                                                        default:
                                                        case value_types::dynamic:
                                                            return BinaryenUnreachable(mod);
                                                        }

                                                        return BinaryenReturnCall(mod,
                                                                                  func,
                                                                                  std::data(args),
                                                                                  std::size(args),
                                                                                  BinaryenTypeNone());
                                                    })));
    }
}

expr_ref compiler::operator()(const table_constructor& p)
{
    expr_ref_list exp;
    expression_list array_init;
    for (auto& field : p)
    {
        std::visit(overload{
                       [&](const std::monostate&)
                       {
                           array_init.push_back(field.value);
                       },
                       [&](const expression& index)
                       {
                           exp.push_back((*this)(index));
                           exp.push_back((*this)(field.value));
                       },
                       [&](const name_t& name)
                       {
                           exp.push_back(add_string(name));
                           exp.push_back((*this)(field.value));
                       },
                   },
                   field.index);
    }

    auto array = (*this)(array_init);

    expr_ref table_init[] = {

        array,
        BinaryenArrayNewFixed(mod, BinaryenTypeGetHeapType(ref_array_type()), std::data(exp), std::size(exp)),
    };
    return BinaryenStructNew(mod, std::data(table_init), std::size(table_init), BinaryenTypeGetHeapType(type<value_types::table>()));
}
} // namespace wumbo