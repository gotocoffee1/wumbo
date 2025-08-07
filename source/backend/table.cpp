#include "compiler.hpp"

namespace wumbo
{

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
    auto tbl = help_var_scope{_func_stack, get_type<table>()};
    exp.push_back(nullptr);
    for (auto& field : p)
    {
        std::visit(overload{
                       [&](const std::monostate&)
                       {
                           array_init.push_back(field.value);
                       },
                       [&](const expression& index)
                       {
                           exp.push_back(table_set(local_get(tbl, get_type<table>()), (*this)(index), (*this)(field.value)));
                       },
                       [&](const name_t& name)
                       {
                           exp.push_back(table_set(local_get(tbl, get_type<table>()), add_string(name), (*this)(field.value)));
                       },
                   },
                   field.index);
    }

    if (!array_init.empty())
    {
        auto array = (*this)(array_init);
        exp[0]     = local_set(tbl, _runtime.call(functions::table_create_array, std::array{
                                                                                 array,
                                                                             }));
    }
    else
        exp[0] = local_set(tbl, _runtime.call(functions::table_create_map, std::array{
                                                                               const_i32(exp.size() - 1),
                                                                           }));
    exp.push_back(local_get(tbl, get_type<table>()));
    return make_block(exp);
}
} // namespace wumbo
