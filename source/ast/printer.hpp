#pragma once

#include "ast/ast.hpp"
#include "utils/util.hpp"
#include <ostream>
#include <string_view>

namespace wumbo::ast
{
struct printer
{
    std::ostream& out;

    std::size_t _indent = 0;

    static constexpr std::size_t per_indentation = 4;

    struct guard
    {
        printer& p;

        ~guard()
        {
            p._indent--;
        }
    };

    void print(std::string_view name)
    {
        out << std::string(_indent * per_indentation, ' ') << name << "\n";
    }

    guard print_indent(std::string_view name)
    {
        print(name);
        _indent++;
        return guard{*this};
    }

    void _functail(const functail& f)
    {
        (*this)(f.args);
        //(*this)(f.name);
    }

    void _vartail(const vartail& v)
    {
        std::visit(overload{
                       [&](const expression& exp)
                       {
                           (*this)(exp);
                       },
                       [&](const name_t& name)
                       {
                           print(name);
                       },
                   },
                   v);
    }

    void _varhead(const varhead& v)
    {
        std::visit(overload{
                       [&](const std::pair<expression, vartail>& exp)
                       {
                           (*this)(exp.first);
                           _vartail(exp.second);
                       },
                       [&](const name_t& name)
                       {
                           print(name);
                       },
                   },
                   v);
    }

    void _funchead(const funchead& v)
    {
        std::visit(overload{
                       [&](const expression& exp)
                       {
                           (*this)(exp);
                       },
                       [&](const name_t& name)
                       {
                           print(name);
                       },
                   },
                   v);
    }

    void visit(const assignments& p)
    {
        auto g = print_indent("assignments");
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

        (*this)(p.explist);
    }

    void visit(const function_call& p)
    {
        auto g = print_indent("function_call");

        _funchead(p.head);

        for (auto& [var, func] : p.tail)
        {
            for (auto& v : var)
                _vartail(v);
            _functail(func);
        }
    }

    void visit(const prefixexp& p)
    {
        auto g = print_indent("prefixexp");

        _funchead(p.chead);

        for (auto& t : p.tail)
        {
            std::visit(overload{
                           [&](const functail& f)
                           {
                               _functail(f);
                           },
                           [&](const vartail& v)
                           {
                               _vartail(v);
                           },
                       },
                       t);
        }
    }

    void visit(const label_statement& p)
    {
        print("label_statement");
    }
    void visit(const key_break& p)
    {
        print("key_break");
    }
    void visit(const goto_statement& p)
    {
        print("goto_statement");
    }
    void visit(const do_statement& p)
    {
        auto g = print_indent("do_statement");
    }
    void visit(const while_statement& p)
    {
        auto g = print_indent("while_statement");
    }
    void visit(const repeat_statement& p)
    {
        auto g = print_indent("repeat_statement");
    }
    void visit(const if_statement& p)
    {
        auto g = print_indent("if_statement");
    }
    void visit(const for_statement& p)
    {
        auto g = print_indent("for_statement");
    }
    void visit(const for_each& p)
    {
        auto g = print_indent("for_each");
    }
    void visit(const function_definition& p)
    {
        auto g = print_indent("function_definition");

        (*this)(p.body);
    }
    void visit(const local_function& p)
    {
        auto g = print_indent("local_function");
    }
    void visit(const local_variables& p)
    {
        auto g = print_indent("local_variables");
        (*this)(p.names);
        (*this)(p.explist);
    }

    void visit(const expression& p)
    {
        std::visit(*this, p.inner);
    }

    void visit(const expression_list& p)
    {
        for (auto& exp : p)
        {
            (*this)(exp);
        }
    }

    void visit(const nil& p)
    {
        print("nil");
    }
    void visit(const boolean& p)
    {
        print(p.value ? "true" : "false");
    }
    void visit(const int_type& p)
    {
        print(std::to_string(p));
    }
    void visit(const float_type& p)
    {
        print(std::to_string(p));
    }
    void visit(const literal& p)
    {
        print("\"" + p.str + "\"");
    }
    void visit(const ellipsis& p)
    {
        print("...");
    }
    void visit(const function_body& p)
    {
        auto g = print_indent("function_body");
        (*this)(p.params);
        if (p.vararg)
            print("...");

        (*this)(p.inner);
    }

    void visit(const table_constructor& p)
    {
        auto g = print_indent("table_constructor");
        for (auto& field : p)
        {
            auto g = print_indent("field");
            std::visit(overload{
                           [](const std::monostate&)
                           {
                               // array
                           },
                           [&](const expression& exp)
                           {
                               (*this)(exp);
                           },
                           [&](const name_t& name)
                           {
                               print(name);
                           },
                       },
                       field.index);
            (*this)(field.value);
        }
    }
    void visit(const bin_operation& p)
    {
        auto g = print_indent("bin_operation");
        (*this)(p.lhs);
        (*this)(p.rhs);
    }
    void visit(const un_operation& p)
    {
        auto g = print_indent("un_operation");
        (*this)(p.rhs);
    }

    template<typename T>
    void operator()(const box<T>& p)
    {
        visit(*p);
    }

    template<typename T>
    void operator()(const T& p)
    {
        visit(p);
    }

    void visit(const name_list& p)
    {
        for (auto& n : p)
        {
            print(n);
        }
    }

    void visit(const block& p)
    {
        auto g = print_indent("block");
        for (auto& statement : p.statements)
        {
            std::visit(*this, statement.inner);
        }
        if (p.retstat)
        {
            print("return");
            (*this)(*p.retstat);
        }
    }
};
} // namespace wumbo::ast
