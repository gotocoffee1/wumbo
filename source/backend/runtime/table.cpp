#include "runtime.hpp"

#include "binaryen-c.h"
#include "backend/wasm_util.hpp"
#include "utils/type.hpp"

namespace wumbo
{
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

}
