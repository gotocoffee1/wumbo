#pragma once

#include "ast/ast.hpp"
#include "ast/func_stack.hpp"
#include "utils/util.hpp"

namespace wumbo::ast
{
struct analyze
{
    function_stack _func_stack;

    void _functail(functail& f)
    {
        visit(f.args);
        //visit(f.name);
    }

    void _vartail(vartail& v)
    {
        std::visit(overload{
                       [&](expression& exp)
                       {
                           visit(exp);
                       },
                       [&](const name_t& name) {

                       },
                   },
                   v);
    }

    void _varhead(varhead& v)
    {
        std::visit(overload{
                       [&](std::pair<expression, vartail>& exp)
                       {
                           visit(exp.first);
                           _vartail(exp.second);
                       },
                       [&](const name_t& name)
                       {
                           get_var(name);
                       },
                   },
                   v);
    }

    void _funchead(funchead& v)
    {
        std::visit(overload{
                       [&](expression& exp)
                       {
                           visit(exp);
                       },
                       [&](const name_t& name)
                       {
                           get_var(name);
                       },
                   },
                   v);
    }

    void _vartail_set(vartail& p)
    {
        std::visit(overload{
                       [&](expression& exp)
                       {
                           (*this)(exp);
                           //return table_set(var, , value);
                       },
                       [&](const name_t& name)
                       {
                           //return table_set(var, add_string(name), value);
                       },
                   },
                   p);
    }

    void _varhead_set(varhead& v)
    {
        std::visit(overload{
                       [&](std::pair<expression, vartail>& exp)
                       {
                           (*this)(exp.first);
                           _vartail_set(exp.second);
                       },
                       [&](const name_t& name)
                       {
                           set_var(name);
                       },
                   },
                   v);
    }

    void visit(assignments& p)
    {
        for (auto& var : p.varlist)
        {
            _varhead(var.head);
            for (auto& [func, var] : var.tail)
            {
                for (auto& f : func)
                    _functail(f);
                _vartail(var);
            }
        }

        visit(p.explist);
    }

    void visit(function_call& p)
    {
        _funchead(p.head);

        for (auto& [var, func] : p.tail)
        {
            for (auto& v : var)
                _vartail(v);
            _functail(func);
        }
    }

    void visit(prefixexp& p)
    {
        _funchead(p.chead);

        for (auto& t : p.tail)
        {
            std::visit(overload{
                           [&](functail& f)
                           {
                               _functail(f);
                           },
                           [&](vartail& v)
                           {
                               _vartail(v);
                           },
                       },
                       t);
        }
    }

    void visit(label_statement& p)
    {
    }
    void visit(key_break& p)
    {
    }
    void visit(goto_statement& p)
    {
    }
    void visit(do_statement& p)
    {
    }
    void visit(while_statement& p)
    {
    }
    void visit(repeat_statement& p)
    {
    }
    void visit(if_statement& p)
    {
    }
    void visit(for_statement& p)
    {
    }
    void visit(for_each& p)
    {
    }
    void visit(function_definition& p)
    {
        visit(p.body);
    }
    void visit(local_function& p)
    {
        _func_stack.alloc_local(p.name, p.usage);
        visit(p.body);
    }
    void visit(local_variables& p)
    {
        visit(p.explist);
        p.usage.resize(p.names.size());
        size_t i = 0;
        for (auto& n : p.names)
            _func_stack.alloc_local(n, p.usage[i++]);
    }

    void visit(expression& p)
    {
        std::visit(*this, p.inner);
    }

    void visit(expression_list& p)
    {
        for (auto& exp : p)
        {
            visit(exp);
        }
    }

    void visit(nil& p)
    {
    }
    void visit(boolean& p)
    {
    }
    void visit(int_type& p)
    {
    }
    void visit(float_type& p)
    {
    }
    void visit(literal& p)
    {
    }
    void visit(ellipsis& p)
    {
    }
    void visit(function_body& p)
    {
        function_frame f{_func_stack};
        p.usage.resize(p.params.size());
        size_t i = 0;
        for (auto& n : p.params)
            _func_stack.alloc_local(n, p.usage[i++]);

        visit(p.inner);
    }

    void visit(table_constructor& p)
    {
        for (auto& field : p)
        {
            std::visit(overload{
                           [](std::monostate&)
                           {
                               // array
                           },
                           [&](expression& exp)
                           {
                               visit(exp);
                           },
                           [&](name_t& name) {

                           },
                       },
                       field.index);
            visit(field.value);
        }
    }
    void visit(bin_operation& p)
    {
        visit(p.lhs);
        visit(p.rhs);
    }

    void visit(un_operation& p)
    {
        visit(p.rhs);
    }

    template<typename T>
    void operator()(box<T>& p)
    {
        visit(*p);
    }

    template<typename T>
    void operator()(T& p)
    {
        visit(p);
    }

    void visit(block& p)
    {
        block_scope b{_func_stack};
        for (auto& statement : p.statements)
        {
            std::visit(*this, statement.inner);
        }
        if (p.retstat)
        {
            visit(*p.retstat);
        }
    }

    void set_var(const name_t& name)
    {
        auto [var_type, usage] = _func_stack.find(name);
        switch (var_type)
        {
        case var_type::upvalue:
            usage->upvalue = true;
            [[fallthrough]];
        case var_type::local:
            usage->write_count++;
            break;
        case var_type::global:
            get_var("_ENV");
            break;
        default:
            break;
        }
    }

    void get_var(const name_t& name)
    {
        auto [var_type, usage] = _func_stack.find(name);
        switch (var_type)
        {
        case var_type::upvalue:
            usage->upvalue = true;
            [[fallthrough]];
        case var_type::local:
            usage->read_count++;
            break;
        case var_type::global:
            assert(name != "_ENV" && "no environment set");
            get_var("_ENV");
            break;
        default:
            break;
        }
    }
};
} // namespace wumbo::ast