#include "runtime.hpp"

#include "binaryen-c.h"
#include "wasm_util.hpp"

namespace wumbo
{
void runtime::build()
{
    build_types();
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

void runtime::func_table_get()
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
                                                    [&](value_type type, expr_ref exp)
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
                                                    })));
    }
}
} // namespace wumbo
