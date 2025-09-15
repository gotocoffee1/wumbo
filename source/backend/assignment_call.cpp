#include "compiler.hpp"

namespace wumbo
{

expr_ref compiler::call(expr_ref func, expr_ref args)
{
    auto bundle_args = std::array{func, args};

    return _runtime.call(functions::invoke, bundle_args);
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
    if (BinaryenExpressionGetType(function) == ref_array_type())
    {
        auto local = help_var_scope{_func_stack, ref_array_type()};
        function   = at_or_null(local, 0, function);
    }
    expr_ref_list args;

    if (p.name)
    {
        // see _vartail
        auto local = help_var_scope{_func_stack, anyref()};
        auto tbl   = local_tee(local, function, anyref());
        function   = table_get(tbl, add_string(*p.name));

        args.push_back(local_get(local, anyref()));
    }

    for (auto& e : p.args)
        args.push_back((*this)(e));

    return call(function, make_ref_array(args));
}

expr_ref compiler::_vartail(const vartail& p, expr_ref var)
{
    if (BinaryenExpressionGetType(var) == ref_array_type())
    {
        auto local = help_var_scope{_func_stack, ref_array_type()};
        var        = at_or_null(local, 0, var);
    }
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

expr_ref_list compiler::operator()(const function_definition& p)
{
    auto function = add_func_ref(p.function_name.back().c_str(), p.body);
    expr_ref result;
    if (p.function_name.size() == 1)
        result = set_var(p.function_name.front(), function);
    else
    {
        auto tbl = get_var(p.function_name.front());
        for (size_t i = 1; i < (p.function_name.size() - 1); ++i)
            tbl = table_get(tbl, add_string(p.function_name[i]));

        result = table_set(tbl, add_string(p.function_name.back()), function);
    }
    return {result};
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
    return expr_ref_list{drop(exp)};
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
