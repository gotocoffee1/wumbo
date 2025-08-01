#include "compiler.hpp"

namespace wumbo
{

expr_ref compiler::find_bucket(expr_ref table, expr_ref hash)
{
    auto hash_map = BinaryenStructGet(mod, tbl_hash_index, table, ref_array_type(), false);

    auto bucket_index = BinaryenBinary(mod, BinaryenRemUInt32(), hash, array_len(hash_map));
    auto bucket       = BinaryenArrayGet(mod, hash_map, bucket_index, ref_array_type(), false);

    return bucket;
}

expr_ref compiler::table_get(expr_ref table, expr_ref key)
{
    return _runtime.call(functions::table_get,
                         std::array{
                             table,
                             key,
                         });
}

expr_ref compiler::table_set(expr_ref table, expr_ref key, expr_ref value)
{
    return _runtime.call(functions::table_set,
                         std::array{
                             table,
                             key,
                             value,
                         });
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


    std::vector<expr_ref> temp(30, null());
    return table::create(*this, std::array{
                                   array,
                                   hash_array::create_fixed(*this, temp),
                                //    hash_array::create_fixed(*this, exp),
                                   const_i32(0),
                                   null(),
                               });
}
} // namespace wumbo
