#include "runtime.hpp"

#include "backend/wasm_util.hpp"
#include "binaryen-c.h"
#include "utils/type.hpp"

namespace wumbo
{
struct runtime::tbl
{
    static auto hash(runtime* self, value_type vtype)
    {
        auto mod = self->mod;
        runtime::function_stack stack{mod};

        return stack.add_function(("*hash_"s + type_name(vtype)).c_str(), self->size_type(), [&](runtime::function_stack& stack)
                                  {
                                      auto key = stack.alloc(self->type(vtype), "key");
                                      stack.locals();

                                      auto hash = [&]()
                                      {
                                          switch (vtype)
                                          {
                                          case value_type::integer:
                                              return std::vector{self->integer_to_size(self->unbox_integer(stack.get(key)))};
                                          case value_type::number:
                                              // BinaryenTruncSFloat64ToInt32
                                              return std::vector{BinaryenUnreachable(mod)};
                                          case value_type::string:
                                          {
                                              auto h       = stack.alloc(self->size_type(), "h");
                                              auto len     = stack.alloc(self->size_type(), "len");
                                              auto step    = stack.alloc(self->size_type(), "step");
                                              auto tee_len = stack.tee(len, self->array_len(stack.get(key)));
                                              return std::vector{
                                                  // unsigned int h = seed ^ (unsigned int)len;
                                                  stack.set(h, self->binop(BinaryenXorInt32(), tee_len, self->const_i32(0x3eb1b260))),
                                                  // size_t step = (len >> 5) + 1;
                                                  stack.set(step, self->binop(BinaryenAddInt32(), self->binop(BinaryenShrSInt32(), stack.get(len), self->const_i32(5)), self->const_i32(1))),
                                                  BinaryenLoop(mod,
                                                               "+loop",
                                                               self->make_block(std::array{

                                                                   // while (l >= step)
                                                                   self->make_if(self->binop(BinaryenGeUInt32(), stack.get(len), stack.get(step)),
                                                                                 self->make_block(std::array{
                                                                                     // h = h ^ ((h << 5) + (h >> 2) + (unsigned char)(str[l - 1]));
                                                                                     stack.set(h,
                                                                                               self->binop(BinaryenXorInt32(),
                                                                                                           stack.get(h),
                                                                                                           self->binop(BinaryenAddInt32(),
                                                                                                                       self->binop(BinaryenAddInt32(),
                                                                                                                                   self->binop(BinaryenShlInt32(),
                                                                                                                                               stack.get(h),
                                                                                                                                               self->const_i32(5)),
                                                                                                                                   self->binop(BinaryenShrUInt32(),
                                                                                                                                               stack.get(h),
                                                                                                                                               self->const_i32(2))),
                                                                                                                       string::get(*self,
                                                                                                                                   stack.get(key),
                                                                                                                                   self->binop(BinaryenSubInt32(), stack.get(len), self->const_i32(1)))))),
                                                                                     // len -= step;
                                                                                     stack.set(len, self->binop(BinaryenSubInt32(), stack.get(len), stack.get(step))),
                                                                                     BinaryenBreak(mod, "+loop", nullptr, nullptr),
                                                                                 })),
                                                               })),
                                                  // return h;
                                                  stack.get(h),
                                              };
                                          }
                                          case value_type::nil:
                                          case value_type::boolean:
                                          case value_type::function:
                                          case value_type::userdata:
                                          case value_type::thread:
                                          case value_type::table:
                                          default:
                                              return std::vector{BinaryenUnreachable(mod)};
                                          };
                                      };

                                      return self->make_block(hash());
                                  });
    }

    static expr_ref
    calc_pos(runtime* self, expr_ref len, expr_ref hash_value)
    {
        // TODO: use bin and
        return self->binop(BinaryenRemUInt32(),
                           hash_value,
                           len);
    }

    static auto get_distance(runtime* self)
    {
        auto mod = self->mod;
        runtime::function_stack stack{mod};

        return stack.add_function("*get_distance", self->size_type(), [&](runtime::function_stack& stack)
                                  {
                                      auto hash_map = stack.alloc(self->get_type<hash_array>(), "hash_map");
                                      auto element  = stack.alloc(self->get_type<hash_entry>(), "element");
                                      auto pos      = stack.alloc(self->size_type(), "pos");
                                      stack.locals();

                                      auto len = stack.alloc(self->size_type(), "len");

                                      auto hash_value = hash_entry::get<hash_entry::hash>(*self, stack.get(element));
                                      auto best_pos   = calc_pos(self, stack.get(len), hash_value);
                                      return self->make_block(std::array{

                                          calc_pos(self, self->binop(BinaryenSubInt32(), self->binop(BinaryenAddInt32(), stack.get(pos), stack.get(len)), best_pos), stack.tee(len, self->array_len(stack.get(hash_map)))),
                                      });
                                  });
    }

    static auto map_insert_with_hint(runtime* self)
    {
        auto mod = self->mod;
        runtime::function_stack stack{mod};

        return stack.add_function("*map_insert_with_hint", BinaryenTypeNone(), [&](runtime::function_stack& stack)
                                  {
                                      auto hash_map = stack.alloc(self->get_type<hash_array>(), "hash_map");
                                      auto new_ele  = stack.alloc(self->hash_entry_type(), "new_ele");
                                      auto pos      = stack.alloc(self->size_type(), "pos");
                                      auto dist     = stack.alloc(self->size_type(), "dist");

                                      stack.locals();
                                      size_t ele    = stack.alloc(self->hash_entry_type(), "ele");
                                      auto ele_dist = stack.alloc(self->size_type(), "ele_dist");
                                      auto capacity = stack.alloc(self->size_type(), "capacity");
                                      auto i        = stack.alloc(self->size_type(), "i");

                                      auto get_distance_func = get_distance(self);

                                      return self->make_block(std::array{
                                          stack.set(capacity, self->array_len(stack.get(hash_map))),

                                          BinaryenLoop(mod,
                                                       "+loop",
                                                       self->make_block(std::array{

                                                           self->make_if(BinaryenRefIsNull(mod, stack.tee(ele, hash_array::get(*self, stack.get(hash_map), stack.get(pos)))),

                                                                         self->make_block(std::array{
                                                                             hash_array::set(*self, stack.get(hash_map), stack.get(pos), stack.get(new_ele)),
                                                                             self->make_return(),
                                                                         })),
                                                           // auto ele_dist = get_distance(hash_map, ele, pos);
                                                           // if (ele_dist < dist)
                                                           self->make_if(self->binop(BinaryenLtUInt32(), stack.tee(ele_dist, get_distance_func(std::array{stack.get(hash_map), stack.get(ele), stack.get(pos)})), stack.get(dist)),
                                                                         self->make_block(std::array{
                                                                             // swap
                                                                             hash_array::set(*self, stack.get(hash_map), stack.get(pos), stack.get(new_ele)),
                                                                             stack.set(new_ele, stack.get(ele)),
                                                                             // dist = ele_dist
                                                                             stack.set(dist, stack.get(ele_dist)),
                                                                         })),
                                                           // pos = calc_pos(pos + 1)
                                                           stack.set(pos, calc_pos(self, stack.get(capacity), self->binop(BinaryenAddInt32(), stack.get(pos), self->const_i32(1)))),

                                                           // dist++
                                                           stack.set(dist, self->binop(BinaryenAddInt32(), stack.get(dist), self->const_i32(1))),
                                                           BinaryenBreak(mod, "+loop", nullptr, nullptr),

                                                       })),

                                      });
                                  });
    }

    static auto map_insert(runtime* self)
    {
        auto mod = self->mod;
        runtime::function_stack stack{mod};

        return stack.add_function("*map_insert", BinaryenTypeNone(), [&](runtime::function_stack& stack)
                                  {
                                      auto hash_map = stack.alloc(self->get_type<hash_array>(), "hash_map");
                                      auto new_ele  = stack.alloc(self->hash_entry_type(), "new_ele");

                                      stack.locals();

                                      return map_insert_with_hint(self)(std::array{
                                                                            stack.get(hash_map),
                                                                            stack.get(new_ele),

                                                                            calc_pos(self, self->array_len(stack.get(hash_map)), hash_entry::get<hash_entry::hash>(*self, stack.get(new_ele))),
                                                                            self->const_i32(0),
                                                                        },
                                                                        true);
                                  });
    }

    static auto map_resize(runtime* self)
    {
        auto mod = self->mod;
        runtime::function_stack stack{mod};

        return stack.add_function("*map_grow", BinaryenTypeNone(), [&](runtime::function_stack& stack)
                                  {
                                      auto tbl = stack.alloc(self->get_type<table>(), "table");
                                      stack.locals();
                                      auto hash_map     = stack.alloc(self->get_type<hash_array>(), "hash_map");
                                      auto new_hash_map = stack.alloc(self->get_type<hash_array>(), "new_hash_map");
                                      auto capacity     = stack.alloc(self->size_type(), "capacity");
                                      auto ele          = stack.alloc(self->get_type<hash_entry>(), "ele");

                                      auto tee_hash_map     = stack.tee(hash_map, table::get<table::hash>(*self, stack.get(tbl)));
                                      auto tee_capacity     = stack.tee(capacity, self->array_len(tee_hash_map));
                                      auto tee_capacity_dec = stack.tee(capacity, self->binop(BinaryenSubInt32(), stack.get(capacity), self->const_i32(1)));
                                      auto tee_new_hash_map = stack.tee(new_hash_map, hash_array::create(*self, self->binop(BinaryenMulInt32(), tee_capacity, self->const_i32(2))));
                                      auto insert           = map_insert(self);
                                      return self->make_block(std::array{
                                          table::set<table::hash>(*self, stack.get(tbl), tee_new_hash_map),
                                          BinaryenLoop(mod,
                                                       "+loop",
                                                       self->make_block(std::array{
                                                           BinaryenBreak(mod, "+loop", BinaryenRefIsNull(mod, stack.tee(ele, hash_array::get(*self, stack.get(hash_map), tee_capacity_dec))), nullptr),
                                                           insert(std::array{
                                                               stack.get(new_hash_map),
                                                               stack.get(ele),
                                                           }),
                                                           BinaryenBreak(mod, "+loop", stack.get(capacity), nullptr),
                                                       })),
                                      });
                                  });
    }

    static auto set(runtime* self, value_type vtype)
    {
        auto mod = self->mod;

        runtime::function_stack stack{mod};

        auto set = [&](runtime::function_stack& stack)
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
            size_t new_ele    = stack.alloc(self->hash_entry_type(), "new_ele");
            size_t capacity   = stack.alloc(self->size_type(), "capacity");
            size_t size       = stack.alloc(self->size_type(), "size");

            auto tee_hash_map = stack.tee(hash_map, table::get<table::hash>(*self, stack.get(table)));

            auto tee_hash_value = stack.tee(hash_value, hash(self, vtype)(std::array{stack.get(key)}));
            auto tee_capacity   = stack.tee(capacity, self->array_len(tee_hash_map));
            auto tee_size       = stack.tee(size, table::get<table::hash_size>(*self, stack.get(table)));

            auto get_distance_func = get_distance(self);
            auto map_resize_func   = map_resize(self);
            auto set_ele           = [&]()
            {
                return hash_array::set(*self, stack.get(hash_map), stack.get(pos), stack.get(new_ele));
            };
            auto inc_size = [&]()
            {
                return table::set<table::hash_size>(*self, stack.get(table), self->binop(BinaryenAddInt32(), stack.get(size), self->const_i32(1)));
            };

            auto body = std::array{
                // if (size > capacity * max_load_factor)
                self->make_if(self->binop(BinaryenGtFloat32(), self->unop(BinaryenConvertUInt32ToFloat32(), tee_size), self->binop(BinaryenMulFloat32(), self->unop(BinaryenConvertUInt32ToFloat32(), tee_capacity), BinaryenConst(mod, BinaryenLiteralFloat32(0.8f)))),
                              self->make_block(std::array{
                                  // resize
                                  map_resize_func(std::array{stack.get(table)}),
                                  stack.set(capacity, self->array_len(stack.tee(hash_map, table::get<table::hash>(*self, stack.get(table))))),
                              })),

                stack.set(pos, calc_pos(self, stack.get(capacity), tee_hash_value)),
                stack.set(new_ele, hash_entry::create(*self, std::array{stack.get(key), stack.get(value), stack.get(hash_value)})),
                BinaryenLoop(mod,
                             "+loop",
                             self->make_block(std::array{

                                 self->make_if(BinaryenRefIsNull(mod, stack.tee(ele, hash_array::get(*self, stack.get(hash_map), stack.get(pos)))),
                                               self->make_block(std::array{
                                                   set_ele(),
                                                   inc_size(),
                                                   self->make_return(),
                                               })),
                                 self->make_if(self->binop(BinaryenEqInt32(), stack.get(hash_value), hash_entry::get<hash_entry::hash>(*self, stack.get(ele))),
                                               self->make_if(self->compare(vtype)(std::array{stack.get(key), hash_entry::get<hash_entry::key>(*self, stack.get(ele))}),
                                                             self->make_block(std::array{
                                                                 set_ele(),
                                                                 self->make_return(),
                                                             }))),
                                 // auto ele_dist = get_distance(hash_map, ele, pos);
                                 // if (ele_dist < dist)
                                 self->make_if(self->binop(BinaryenLtUInt32(), stack.tee(ele_dist, get_distance_func(std::array{stack.get(hash_map), stack.get(ele), stack.get(pos)})), stack.get(dist)),
                                               self->make_block(std::array{
                                                   // swap
                                                   set_ele(),
                                                   inc_size(),
                                                   map_insert_with_hint(self)(std::array{
                                                                                  stack.get(hash_map),
                                                                                  stack.get(ele),
                                                                                  calc_pos(self, stack.get(capacity), self->binop(BinaryenAddInt32(), stack.get(pos), self->const_i32(1))),
                                                                                  self->binop(BinaryenAddInt32(), stack.get(ele_dist), self->const_i32(1)),
                                                                              },
                                                                              true),
                                               })),
                                 // pos = calc_pos(pos + 1)
                                 stack.set(pos, calc_pos(self, stack.get(capacity), self->binop(BinaryenAddInt32(), stack.get(pos), self->const_i32(1)))),

                                 // dist++
                                 stack.set(dist, self->binop(BinaryenAddInt32(), stack.get(dist), self->const_i32(1))),

                                 BinaryenBreak(mod, "+loop", nullptr, nullptr),

                             })),
            };

            return self->make_block(body);
        };

        return stack.add_function(("*table_set_"s + type_name(vtype)).c_str(), BinaryenTypeNone(), set);
    }

    static auto get(runtime* self, value_type vtype)
    {
        auto mod = self->mod;

        runtime::function_stack stack{mod};

        auto get = [&](runtime::function_stack& stack)
        {
            auto table = stack.alloc(self->type<value_type::table>(), "table");
            auto key   = stack.alloc(self->type(vtype), "key");
            stack.locals();
            auto o = BinaryenNop(mod);
            if (vtype == value_type::integer)
            {
                // TODO check 0xFF00000001
                auto i     = stack.alloc(self->size_type(), "i");
                auto tee_i = stack.tee(i, self->binop(BinaryenSubInt32(), self->integer_to_size(integer::get<integer::inner>(*self, stack.get(key))), self->const_i32(1)));
                o          = self->make_if(self->binop(BinaryenLtUInt32(), tee_i, table::get<table::array_size>(*self, stack.get(table))), self->make_return(ref_array::get(*self, table::get<table::array>(*self, stack.get(table)), stack.get(i))));
                stack.free_local(i);
            }

            auto hash_map   = stack.alloc(self->hash_array_type(), "hash_map");
            auto hash_value = stack.alloc(self->size_type(), "hash_value");
            auto dist       = stack.alloc(self->size_type(), "dist");
            auto pos        = stack.alloc(self->size_type(), "pos");
            auto ele_dist   = stack.alloc(self->size_type(), "ele_dist");
            auto ele        = stack.alloc(self->hash_entry_type(), "ele");

            auto tee_hash_map   = stack.tee(hash_map, table::get<table::hash>(*self, stack.get(table)));
            auto tee_hash_value = stack.tee(hash_value, hash(self, vtype)(std::array{stack.get(key)}));

            auto get_distance_func = get_distance(self);

            auto body = std::array{
                o,
                stack.set(pos, calc_pos(self, self->array_len(tee_hash_map), tee_hash_value)),

                BinaryenLoop(mod,
                             "+loop",
                             self->make_block(std::array{

                                 self->make_if(BinaryenRefIsNull(mod, stack.tee(ele, hash_array::get(*self, stack.get(hash_map), stack.get(pos)))),

                                               self->make_block(std::array{

                                                   // return null;
                                                   self->make_return(self->null()),
                                               })),
                                 // auto ele_dist = get_distance(hash_map, ele, pos);
                                 // if (ele_dist < dist)
                                 self->make_if(self->binop(BinaryenLtUInt32(), stack.tee(ele_dist, get_distance_func(std::array{stack.get(hash_map), stack.get(ele), stack.get(pos)})), stack.get(dist)),
                                               self->make_block(std::array{
                                                   // return null;
                                                   self->make_return(self->null()),

                                               })),
                                 // if (ele.hash == hash_value && ele.key == key)
                                 self->make_if(self->binop(BinaryenEqInt32(), stack.get(hash_value), hash_entry::get<hash_entry::hash>(*self, stack.get(ele))),
                                               self->make_if(self->compare(vtype)(std::array{stack.get(key), hash_entry::get<hash_entry::key>(*self, stack.get(ele))}),
                                                             self->make_block(std::array{
                                                                 // return ele;
                                                                 self->make_return(hash_entry::get<hash_entry::value>(*self, stack.get(ele))),
                                                             }))),
                                 // pos = calc_pos(pos + 1)
                                 stack.set(pos, calc_pos(self, self->array_len(stack.get(hash_map)), self->binop(BinaryenAddInt32(), stack.get(pos), self->const_i32(1)))),

                                 // dist++
                                 stack.set(dist, self->binop(BinaryenAddInt32(), stack.get(dist), self->const_i32(1))),

                                 BinaryenBreak(mod, "+loop", nullptr, nullptr),

                             })),

            };
            return self->make_block(body);
        };

        return stack.add_function(("*table_get_"s + type_name(vtype)).c_str(), anyref(), get);
    }
};

build_return_t runtime::table_create_array()
{
    return {std::vector<BinaryenType>{},
            make_block(std::array{
                table::create(*this, std::array{
                                         local_get(0, ref_array_type()),
                                         hash_array::create_fixed(*this, std::array{null()}),
                                         array_len(local_get(0, ref_array_type())),
                                         const_i32(0),
                                         null(),
                                     }),
            })};
}

build_return_t runtime::table_create_map()
{
    return {std::vector<BinaryenType>{},
            make_block(std::array{
                table::create(*this, std::array{
                                         null(),
                                         hash_array::create(*this, const_i32(1) /*local_get(0, size_type())*/),
                                         const_i32(0),
                                         const_i32(0),
                                         null(),
                                     }),
            })};
}

build_return_t runtime::table_set()
{
    auto table = local_get(0, anyref());
    auto key   = local_get(1, anyref());
    auto value = local_get(2, anyref());
    table      = BinaryenRefCast(mod, table, type<value_type::table>());

    auto casts = std::array{
        value_type::number,
        value_type::integer,
        value_type::string,
    };
    return {std::vector<BinaryenType>{},
            make_block(switch_value(key, casts, [&](value_type type, expr_ref exp)
                                    {
                                        switch (type)
                                        {
                                        case value_type::boolean:
                                        case value_type::integer:
                                        case value_type::number:
                                        case value_type::string:
                                        case value_type::function:
                                        case value_type::userdata:
                                        case value_type::thread:
                                        case value_type::table:

                                            return tbl::set(this, type)(std::array{table, exp, value}, true);
                                        default:
                                        case value_type::nil:
                                            return BinaryenUnreachable(mod);
                                        }
                                    }))};
}

build_return_t runtime::table_get()
{
    auto table = local_get(0, anyref());
    auto key   = local_get(1, anyref());
    table      = BinaryenRefCast(mod, table, type<value_type::table>());

    auto casts = std::array{
        value_type::number,
        value_type::integer,
        value_type::string,
    };

    return {std::vector<BinaryenType>{},
            make_block(switch_value(key,
                                    casts,
                                    [&](value_type type, expr_ref exp)
                                    {
                                        switch (type)
                                        {
                                        case value_type::boolean:
                                        case value_type::integer:
                                        case value_type::number:
                                        case value_type::string:
                                        case value_type::function:
                                        case value_type::userdata:
                                        case value_type::thread:
                                        case value_type::table:
                                            return tbl::get(this, type)(std::array{table, exp}, true);
                                        default:
                                        case value_type::nil:
                                            return BinaryenUnreachable(mod);
                                        }
                                    }))};
}
} // namespace wumbo
