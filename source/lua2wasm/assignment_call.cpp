#include "compiler.hpp"

namespace wumbo
{

BinaryenExpressionRef compiler::_funchead(const funchead& p)
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

BinaryenExpressionRef compiler::_functail(const functail& p, BinaryenExpressionRef function)
{
    auto t        = type<value_types::function>();
    auto local    = alloc_local(t);
    auto args     = (*this)(p.args);
    auto tee      = BinaryenLocalTee(mod, local, BinaryenRefCast(mod, function, t), t);
    auto func_ref = BinaryenStructGet(mod, 0, local_get(local, t), BinaryenTypeFuncref(), false);
    auto upvalues = BinaryenStructGet(mod, 1, tee, ref_array_type(), false);
    free_local(local);

    BinaryenExpressionRef real_args[] = {
        upvalues,
        args,
    };

    return BinaryenCallRef(mod, func_ref, std::data(real_args), std::size(real_args), BinaryenTypeNone(), false);
}

BinaryenExpressionRef compiler::_vartail(const vartail& p, BinaryenExpressionRef var)
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

BinaryenExpressionRef compiler::_vartail_set(const vartail& p, BinaryenExpressionRef var, BinaryenExpressionRef value)
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

BinaryenExpressionRef compiler::_varhead(const varhead& v)
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

BinaryenExpressionRef compiler::_varhead_set(const varhead& v, BinaryenExpressionRef value)
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

std::vector<BinaryenExpressionRef> compiler::operator()(const assignments& p)
{
    std::vector<BinaryenExpressionRef> result;

    auto local = alloc_local(ref_array_type());
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

    free_local(local);

    return result;
}

std::vector<BinaryenExpressionRef> compiler::operator()(const function_call& p)
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
    return std::vector<BinaryenExpressionRef>{BinaryenDrop(mod, exp)};
}

BinaryenExpressionRef compiler::operator()(const prefixexp& p)
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
