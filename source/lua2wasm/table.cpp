#include "compiler.hpp"

namespace wumbo
{
BinaryenExpressionRef compiler::calc_hash(BinaryenExpressionRef key)
{
    return const_i32(3);
}

BinaryenExpressionRef compiler::find_bucket(BinaryenExpressionRef table, BinaryenExpressionRef hash)
{
    auto hash_map = BinaryenStructGet(mod, tbl_hash_index, table, ref_array_type(), false);

    auto bucket_index = BinaryenBinary(mod, BinaryenRemUInt32(), hash, array_len(hash_map));
    auto bucket       = BinaryenArrayGet(mod, hash_map, bucket_index, ref_array_type(), false);

    return bucket;
}

BinaryenExpressionRef compiler::table_get(BinaryenExpressionRef table, BinaryenExpressionRef key)
{
    BinaryenExpressionRef args[] = {
        table,
        key,
    };

    return BinaryenCall(mod, "*table_get", std::data(args), std::size(args), anyref());
}

BinaryenExpressionRef compiler::table_set(BinaryenExpressionRef table, BinaryenExpressionRef key, BinaryenExpressionRef value)
{
    BinaryenExpressionRef args[] = {
        table,
        key,
        value,
    };

    return BinaryenCall(mod, "*table_set", std::data(args), std::size(args), BinaryenTypeNone());
}

BinaryenFunctionRef compiler::compare(const char* name, value_types vtype)
{
    std::vector<std::tuple<const char*, value_types>> casts = {
        {
            name,
            vtype,
        },
    };
    auto exp = local_get(0, type(vtype));

    BinaryenType params[] = {
        type(vtype),
        anyref(),
    };
    std::vector<BinaryenType> vars;

    auto block = make_block(switch_value(local_get(1, anyref()), casts, [&](value_types type_right, BinaryenExpressionRef exp_right)
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
                                                 return BinaryenReturn(mod, BinaryenBinary(mod, BinaryenEqInt64(), BinaryenStructGet(mod, 0, exp, integer_type(), false), BinaryenStructGet(mod, 0, exp_right, integer_type(), false)));
                                             case value_types::number:
                                                 return BinaryenReturn(mod, BinaryenBinary(mod, BinaryenEqFloat64(), BinaryenStructGet(mod, 0, exp, number_type(), false), BinaryenStructGet(mod, 0, exp_right, number_type(), false)));
                                             case value_types::table:
                                             case value_types::function:
                                             case value_types::thread:
                                                 return BinaryenRefEq(mod, exp, exp_right);

                                             case value_types::string:
                                             {
                                                 auto string_t = type<value_types::string>();
                                                 auto i        = vars.size() + std::size(params);
                                                 vars.push_back(BinaryenTypeInt32());
                                                 auto exp_i = vars.size() + std::size(params);
                                                 vars.push_back(string_t);
                                                 auto exp_right_i = vars.size() + std::size(params);
                                                 vars.push_back(string_t);

                                                 auto dec = [&](BinaryenExpressionRef first)
                                                 {
                                                     return BinaryenBinary(mod,
                                                                           BinaryenSubInt32(),
                                                                           first,
                                                                           const_i32(1));
                                                 };

                                                 return BinaryenReturn(mod,
                                                                       BinaryenIf(mod,
                                                                                  BinaryenBinary(mod, BinaryenEqInt32(), local_tee(i, array_len(local_tee(exp_i, exp, string_t)), BinaryenTypeInt32()), array_len(local_tee(exp_right_i, exp_right, string_t))),
                                                                                  BinaryenLoop(mod,
                                                                                               "+loop",
                                                                                               BinaryenIf(mod,
                                                                                                          local_get(i, BinaryenTypeInt32()),
                                                                                                          make_block(std::array{
                                                                                                              BinaryenBreak(mod,
                                                                                                                            "+loop",
                                                                                                                            BinaryenBinary(mod,
                                                                                                                                           BinaryenEqInt32(),
                                                                                                                                           array_get(local_get(exp_i, string_t), local_tee(i, dec(local_get(i, BinaryenTypeInt32())), BinaryenTypeInt32()), BinaryenTypeInt32()),
                                                                                                                                           array_get(local_get(exp_right_i, string_t), local_get(i, BinaryenTypeInt32()), BinaryenTypeInt32())),
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
                            BinaryenTypeInt32());

    return BinaryenAddFunction(mod,
                               (std::string("*key_compare_") + name).c_str(),
                               BinaryenTypeCreate(std::data(params), std::size(params)),
                               BinaryenTypeInt32(),
                               std::data(vars),
                               std::size(vars),
                               block);
}

void compiler::func_table_get()
{
    auto table  = BinaryenLocalGet(mod, 0, anyref());
    auto key    = BinaryenLocalGet(mod, 1, anyref());
    auto value  = BinaryenLocalGet(mod, 2, anyref());
    table       = BinaryenRefCast(mod, table, type<value_types::table>());
    auto bucket = BinaryenStructGet(mod, tbl_hash_index, table, ref_array_type(), false);

    //auto hash = calc_hash(key);

    //auto bucket = find_bucket(table, hash);

    //auto bucket = ;

    auto dec = [&](BinaryenExpressionRef first)
    {
        return BinaryenBinary(mod,
                              BinaryenSubInt32(),
                              first,
                              const_i32(2));
    };
    //(BinaryenExpressionRef[]){            BinaryenLocalSet(mod, i, dec(array_len(mod, bucket)))};

    std::vector<std::tuple<const char*, value_types>> casts = {
        //{
        //    "number",
        //    value_types::number,
        //},
        //{
        //    "integer",
        //    value_types::integer,
        //},
        {
            "string",
            value_types::string,
        },
    };

    for (auto& [name, value] : casts)
        compare(name, value);

    auto result = [&](BinaryenExpressionRef on_hit, BinaryenExpressionRef on_miss, size_t i)
    {
        return switch_value(key,
                            casts,
                            [&](value_types type, BinaryenExpressionRef exp)
                            {
                                std::string loop = "+loop" + std::to_string(label_name++);

                                BinaryenExpressionRef args[] = {
                                    exp,
                                   array_get(bucket, local_tee(i, dec(local_get(i, BinaryenTypeInt32())), BinaryenTypeInt32()), anyref()),
                                };

                                const char* comparator;
                                switch (type)
                                {
                                case value_types::integer:
                                    comparator = "*key_compare_integer";
                                    break;
                                case value_types::number:
                                    comparator = "*key_compare_number";
                                    break;
                                case value_types::string:
                                    comparator = "*key_compare_string";
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

                                return make_block(std::array{
                                    local_set(i, array_len(bucket)),
                                    BinaryenLoop(mod,
                                                 loop.c_str(),
                                                 BinaryenIf(mod,
                                                            local_get(i, BinaryenTypeInt32()),
                                                            make_block(std::array{
                                                                BinaryenBreak(mod,
                                                                              loop.c_str(),
                                                                              BinaryenUnary(mod, BinaryenEqZInt32(), BinaryenCall(mod, comparator, std::data(args), std::size(args), BinaryenTypeInt32())),
                                                                              nullptr),
                                                                on_hit,
                                                            }),
                                                            nullptr)),

                                    on_miss,
                                });
                            });
    };

    {
        BinaryenType params[] = {
            anyref(),
            anyref(),
        };

        BinaryenType locals[] = {
            BinaryenTypeInt32(),
        };

        size_t i = std::size(params);

        BinaryenAddFunction(mod,
                            "*table_get",
                            BinaryenTypeCreate(std::data(params), std::size(params)),
                            anyref(),
                            std::data(locals),
                            std::size(locals),
                            make_block(result(BinaryenReturn(mod,
                                                             BinaryenArrayGet(mod,
                                                                              bucket,
                                                                              BinaryenBinary(mod, BinaryenAddInt32(), local_get(i, BinaryenTypeInt32()), const_i32(1)),
                                                                              anyref(),
                                                                              false)),
                                              BinaryenReturn(mod, null()),
                                              i)));
    }

    {
        BinaryenType params[] = {
            anyref(),
            anyref(),
            anyref(),
        };

        BinaryenType locals[] = {
            BinaryenTypeInt32(),
            ref_array_type(),

        };
        size_t i         = std::size(params);
        size_t new_array = std::size(params) + 1;
        BinaryenAddFunction(mod,
                            "*table_set",
                            BinaryenTypeCreate(std::data(params), std::size(params)),
                            BinaryenTypeNone(),
                            std::data(locals),
                            std::size(locals),
                            make_block(result(
                                BinaryenArraySet(mod,
                                                 bucket,
                                                 BinaryenBinary(mod, BinaryenAddInt32(), local_get(i, BinaryenTypeInt32()), const_i32(1)),
                                                 value),
                                make_block(std::array{
                                    resize_array(new_array, ref_array_type(), bucket, const_i32(2), true),

                                    BinaryenStructSet(mod, tbl_hash_index, table, local_get(new_array, ref_array_type())),
                                    BinaryenArraySet(mod, local_get(new_array, ref_array_type()), const_i32(0), key),
                                    BinaryenArraySet(mod, local_get(new_array, ref_array_type()), const_i32(1), value),
                                    BinaryenReturn(mod, nullptr),
                                }),

                                i)));
    }
}

BinaryenExpressionRef compiler::operator()(const table_constructor& p)
{
    std::vector<BinaryenExpressionRef> exp;
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
                           exp.push_back(new_value((*this)(index)));
                           exp.push_back(new_value((*this)(field.value)));
                       },
                       [&](const name_t& name)
                       {
                           exp.push_back(add_string(name));
                           exp.push_back(new_value((*this)(field.value)));
                       },
                   },
                   field.index);
    }

    auto array = (*this)(array_init);

    BinaryenExpressionRef table_init[] = {

        array,
        //array,
        BinaryenArrayNewFixed(mod, BinaryenTypeGetHeapType(ref_array_type()), std::data(exp), std::size(exp)),
    };
    return BinaryenStructNew(mod, std::data(table_init), std::size(table_init), BinaryenTypeGetHeapType(type<value_types::table>()));
}
} // namespace wumbo