#include "compiler.hpp"

namespace wumbo
{

expr_ref compiler::call(expr_ref func, expr_ref args)
{
    auto t = type<value_types::function>();

    auto local = help_var_scope{_func_stack, t};

    auto func_ref = BinaryenStructGet(mod, 0, local_get(local, t), BinaryenTypeFuncref(), false);
    auto upvalues = BinaryenStructGet(mod, 1, local_tee(local, func, t), ref_array_type(), false);

    expr_ref real_args[2];

    real_args[upvalue_index] = upvalues;
    real_args[args_index]    = args;

    return BinaryenCallRef(mod, func_ref, std::data(real_args), std::size(real_args), BinaryenTypeNone(), false);
}

expr_ref compiler::_funchead(const funchead& p)
{
    return std::visit(overload{
                          [&](const name_t& name)
                          {
                              return get_var(name);
                          },
                          [&](const expression& expr)
                          {
                              return (*this)(expr);
                          },
                      },
                      p);
}

expr_ref compiler::_functail(const functail& p, expr_ref function)
{
    auto t = type<value_types::function>();

    auto args = (*this)(p.args);

    return call(BinaryenRefCast(mod, function, t), args);
}

expr_ref compiler::_vartail(const vartail& p, expr_ref var)
{
    return std::visit(overload{
                          [&](const expression& exp)
                          {
                              return table_get(var, (*this)(exp));
                          },
                          [&](const name_t& name)
                          {
                              return table_get(var, add_string(name));
                          },
                      },
                      p);
    return var;
}

expr_ref compiler::_vartail_set(const vartail& p, expr_ref var, expr_ref value)
{
    return std::visit(overload{
                          [&](const expression& exp)
                          {
                              return table_set(var, (*this)(exp), value);
                          },
                          [&](const name_t& name)
                          {
                              return table_set(var, add_string(name), value);
                          },
                      },
                      p);
    return var;
}

expr_ref compiler::_varhead(const varhead& v)
{
    return std::visit(overload{
                          [&](const std::pair<expression, vartail>& exp)
                          {
                              return _vartail(exp.second, (*this)(exp.first));
                          },
                          [&](const name_t& name)
                          {
                              return get_var(name);
                          },
                      },
                      v);
}

expr_ref compiler::_varhead_set(const varhead& v, expr_ref value)
{
    return std::visit(overload{
                          [&](const std::pair<expression, vartail>& exp)
                          {
                              return _vartail_set(exp.second, (*this)(exp.first), value);
                          },
                          [&](const name_t& name)
                          {
                              return set_var(name, value);
                          },
                      },
                      v);
}

expr_ref_list compiler::operator()(const assignments& p)
{
    expr_ref_list result;

    auto local = help_var_scope{_func_stack, ref_array_type()};
    result.push_back(local_set(local, (*this)(p.explist)));

    size_t i = 0;

    for (auto& var : p.varlist)
    {
        if (var.tail.empty())
            result.push_back(_varhead_set(var.head, at_or_null(local, i++)));
        else
        {
            auto exp = _varhead(var.head);
            for (auto& [func, vartail] : var.tail)
            {
                for (auto& f : func)
                    exp = _functail(f, exp);

                if (&vartail == &var.tail.back().second)
                    result.push_back(_vartail_set(vartail, exp, at_or_null(local, i++)));
                else
                    exp = _vartail(vartail, exp);
            }
        }
    }

    return result;
}

expr_ref_list compiler::operator()(const function_call& p)
{
    auto exp = _funchead(p.head);
    for (auto& [vartails, functail] : p.tail)
    {
        for (auto& var : vartails)
        {
            exp = _vartail(var, exp);
        }

        exp = _functail(functail, exp);
    }
    return expr_ref_list{BinaryenDrop(mod, exp)};
}

expr_ref compiler::operator()(const prefixexp& p)
{
    auto exp = _funchead(p.chead);
    for (auto& tail : p.tail)
    {
        exp = std::visit(overload{
                             [&](const functail& f)
                             {
                                 return _functail(f, exp);
                             },
                             [&](const vartail& v)
                             {
                                 return _vartail(v, exp);
                             },
                         },
                         tail);
    }
    return exp;
}
} // namespace wumbo
