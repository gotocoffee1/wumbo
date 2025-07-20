#include "runtime.hpp"

#include "backend/wasm_util.hpp"
#include "binaryen-c.h"
#include "utils/type.hpp"

namespace wumbo
{
struct runtime::tbl
{
    static auto hash(runtime* self, const char* name, value_type vtype)
    {
        auto mod = self->mod;
        runtime::function_stack stack{mod};

        return stack.add_function(("*hash_"s + name).c_str(), self->size_type(), [&]()
                                  {
                                      auto key = stack.alloc(self->type(vtype), "key");
                                      stack.locals();

                                      auto hash = [&]()
                                      {
                                          switch (vtype)
                                          {
                                          case value_type::integer:
                                              return std::array{self->unbox_integer(stack.get(key))};
                                          case value_type::number:
                                              return std::array{BinaryenUnreachable(mod)};
                                          case value_type::string:
                                              return std::array{BinaryenUnreachable(mod)};
                                          case value_type::nil:
                                          case value_type::boolean:
                                          case value_type::function:
                                          case value_type::userdata:
                                          case value_type::thread:
                                          case value_type::table:
                                          default:
                                              return std::array{BinaryenUnreachable(mod)};
                                          };
                                      };

                                      return self->make_block(hash());
                                  });
    }

    static expr_ref calc_pos(runtime* self, expr_ref hash_map, expr_ref hash_value)
    {
        return self->binop(BinaryenAndInt32(),
                           hash_value,
                           self->array_len(hash_map));
    }

    static auto get_distance(runtime* self)
    {
        auto mod = self->mod;
        runtime::function_stack stack{mod};

        return stack.add_function("*get_distance", self->size_type(), [&]()
                                  {
                                      auto hash_map = stack.alloc(self->get_type<hash_array>(), "hash_map");
                                      auto element  = stack.alloc(self->get_type<hash_entry>(), "element");
                                      auto pos      = stack.alloc(self->size_type(), "pos");
                                      stack.locals();

                                      auto hash_value = hash_entry::members::get<hash_entry::hash>(*self, stack.get(element));
                                      auto best_pos   = calc_pos(self, stack.get(hash_map), hash_value);
                                      return self->make_block(std::array{self->binop(BinaryenSubInt32(), best_pos, stack.get(pos))});
                                  });
    }

    static auto set(runtime* self, const char* name, value_type vtype)
    {
        auto mod = self->mod;

        runtime::function_stack stack{mod};

        auto set = [&]()
        {
            auto table = stack.alloc(self->type<value_type::table>(), "table");
            auto key   = stack.alloc(self->type(vtype), "key");
            auto value = stack.alloc(anyref(), "value");
            stack.locals();

            size_t hash_map   = stack.alloc(self->hash_array_type(), "hash_map");
            size_t hash_value = stack.alloc(self->size_type(), "hash_value");
            size_t dist       = stack.alloc(self->size_type(), "dist");
            size_t pos        = stack.alloc(self->size_type(), "pos");
            size_t ele_dist   = stack.alloc(self->size_type(), "ele_dist");
            size_t ele        = stack.alloc(self->hash_entry_type(), "ele");

            auto tee_hash_map   = stack.tee(hash_map, table::members::get<table::hash>(*self, stack.get(table)));
            auto tee_hash_value = stack.tee(hash_value, self->make_call(("*hash_"s + name).c_str(), stack.get(key), size_type()));

            auto get_distance_func = get_distance(self);

            auto body = std::array{

                stack.set(pos, calc_pos(self, tee_hash_map, tee_hash_value)),
                BinaryenLoop(mod,
                             "+loop",
                             self->make_block(std::array{

                                 self->make_if(BinaryenRefIsNull(mod, stack.tee(ele, hash_array::array::get(*self, stack.get(hash_map), stack.get(pos)))), self->make_return()),
                                 self->make_if(self->binop(BinaryenLtUInt32(), stack.tee(ele_dist, get_distance_func(std::array{stack.get(hash_map), stack.get(ele), stack.get(pos)})), stack.get(dist)),
                                               stack.set(dist, stack.get(ele_dist))),
                                 stack.set(pos, self->binop(BinaryenAddInt32(), stack.get(pos), self->const_i32(1))),
                                 BinaryenBreak(mod, "+loop", nullptr, nullptr),

                             })),

            };

            size_t i         = stack.alloc(self->size_type(), "i");
            size_t new_array = stack.alloc(self->ref_array_type(), "new_array");
            return self->make_block(body);
        };
        // auto body = std::array{
        //     self->local_set(i, self->array_len(bucket)),
        //     BinaryenLoop(mod,
        //                  "+loop",
        //                  self->make_if(self->local_get(i, self->size_type()),
        //                                self->make_block(std::array{
        //                                    BinaryenBreak(mod,
        //                                                  "+loop",
        //                                                  BinaryenUnary(mod, BinaryenEqZInt32(), self->make_call(("*key_compare_"s + name).c_str(), std::array{
        //                                                                                                                                                stack.get(key),
        //                                                                                                                                                self->array_get(bucket, self->local_tee(i, BinaryenBinary(mod, BinaryenSubInt32(), self->local_get(i, self->size_type()), self->const_i32(2)), self->size_type()), anyref()),
        //                                                                                                                                            },
        //                                                                                                         bool_type())),
        //                                                  nullptr),
        //                                    BinaryenArraySet(mod, bucket, BinaryenBinary(mod, BinaryenAddInt32(), self->local_get(i, size_type()), self->const_i32(1)), stack.get(value)),
        //                                    self->make_return(),
        //                                }))),
        //     self->resize_array(new_array, self->ref_array_type(), bucket, self->const_i32(2), true),

        //     BinaryenStructSet(mod, tbl_hash_index, stack.get(table), stack.get(new_array)),
        //     BinaryenArraySet(mod, self->local_get(new_array, self->ref_array_type()), self->const_i32(0), stack.get(key)),
        //     BinaryenArraySet(mod, self->local_get(new_array, self->ref_array_type()), self->const_i32(1), stack.get(value)),
        // };

        return stack.add_function(("*table_set_"s + name).c_str(), BinaryenTypeNone(), set);
    }
};

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

#
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
} // namespace wumbo
