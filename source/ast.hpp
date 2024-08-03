#pragma once

#include "box.hpp"
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace ast
{

using name_t    = std::string;
using name_list = std::vector<name_t>;

struct nil
{
};

struct boolean
{
    bool value;
};

using float_type = double;
using int_type   = std::int64_t;

struct literal
{
    std::string str;
};

struct ellipsis
{
};

struct expression;
using expression_list = std::vector<expression>;
struct statement;

using return_stat = std::optional<expression_list>;
struct block
{
    return_stat retstat;               // size >=0
    std::vector<statement> statements; // size >=0
};

struct function_body
{
    block inner;
    name_list params; // size >=0
    bool vararg = false;
};

struct function_definition
{
    function_body body;
    std::vector<name_t> function_name; // size >=1
};

struct local_function
{
    function_body body;
    name_t name;
};

struct bin_operation;
struct un_operation;
struct prefixexp;
struct field;
using table_constructor = std::vector<field>;

struct expression
{
    std::variant<nil,
                 boolean,
                 int_type,
                 float_type,
                 literal,
                 ellipsis,
                 function_body,
                 box<prefixexp>,
                 table_constructor,
                 box<bin_operation>,
                 box<un_operation>>
        inner;
};

enum class bin_operator : uint_fast8_t
{
    addition,
    subtraction,
    multiplication,
    division,
    division_floor,
    exponentiation,
    modulo,

    logic_and,
    logic_or,

    binary_or,
    binary_and,
    binary_xor,
    binary_right_shift,
    binary_left_shift,

    equality,
    inequality,
    less_than,
    greater_than,
    less_or_equal,
    greater_or_equal,

    concat,
};

enum class un_operator : uint_fast8_t
{
    minus,
    logic_not,
    len,
    binary_not,
};

struct bin_operation
{
    expression rhs;
    expression lhs;
    bin_operator op;
};

struct un_operation
{
    expression rhs;
    un_operator op;
};

struct expr_temp
{
    std::vector<expression> list;
    std::vector<bin_operator> b_op;
    std::optional<un_operator> u_op;
};

using args = expression_list; // size >=0

//   vartail ::= '[' exp ']' | '.' Name
using vartail = std::variant<expression, name_t>;
//   varhead ::= Name | '(' exp ')' vartail
using varhead = std::variant<name_t, std::pair<expression, vartail>>;
//   functail ::= args | ':' Name args
struct functail
{
    std::optional<name_t> name;
    args args;
};
//   funchead ::= Name | '(' exp ')'
using funchead = std::variant<name_t, expression>;

//   function_call ::= funchead [ { vartail } functail ]
struct function_call
{
    using tail_t = std::pair<std::vector<vartail>, functail>;
    funchead head;
    std::vector<tail_t> tail; // size >= 1
};

//   var ::= varhead { { functail } vartail }
struct var
{
    using tail_t = std::pair<std::vector<functail>, vartail>;
    varhead head;
    std::vector<tail_t> tail; // size >= 0
};

struct prefixexp
{
    //   chead ::= '(' exp ')' | Name
    std::variant<name_t, expression> chead;
    //   combined ::= chead { functail | vartail }
    std::vector<std::variant<functail, vartail>> tail;
};

struct assignments
{
    std::vector<var> varlist; // size >=1
    expression_list explist;  // size >=1
};

struct label_statement
{
    name_t name;
};

struct key_break
{
};

struct goto_statement
{
    name_t name;
};

struct do_statement
{
    block inner;
};

struct while_statement
{
    block inner;
    expression condition;
};

struct repeat_statement
{
    block inner;
    expression condition;
};

struct if_statement
{
    std::optional<block> else_block;
    std::vector<std::pair<expression, block>> cond_block;
};

struct for_statement
{
    block inner;
    std::vector<expression> exp; // size == 2 || size == 3
    name_t var;
};

struct for_each
{
    block inner;
    name_list names;         // size >=1
    expression_list explist; // size >=1
};

struct for_combined
{
    name_t first;
    std::variant<for_statement, for_each> condition;
    block inner;
};

struct local_variables
{
    expression_list explist; // size >=0
    name_list names;         // size >=1
};

struct field
{
    using index_t = std::variant<std::monostate, expression, name_t>;
    index_t index;
    expression value;
};

struct statement
{
    std::variant<
        assignments,
        function_call,
        label_statement,
        key_break,
        goto_statement,
        do_statement,
        while_statement,
        repeat_statement,
        if_statement,
        for_statement,
        for_each,
        function_definition,
        local_function,
        local_variables>
        inner;
};
} // namespace ast

#include "util.hpp"
#include <ostream>
#include <string_view>

namespace ast
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

    void operator()(const assignments& p)
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

    void operator()(const function_call& p)
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

    void operator()(const prefixexp& p)
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

    void operator()(const label_statement& p)
    {
        print("label_statement");
    }
    void operator()(const key_break& p)
    {
        print("key_break");
    }
    void operator()(const goto_statement& p)
    {
        print("goto_statement");
    }
    void operator()(const do_statement& p)
    {
        auto g = print_indent("do_statement");
    }
    void operator()(const while_statement& p)
    {
        auto g = print_indent("while_statement");
    }
    void operator()(const repeat_statement& p)
    {
        auto g = print_indent("repeat_statement");
    }
    void operator()(const if_statement& p)
    {
        auto g = print_indent("if_statement");
    }
    void operator()(const for_statement& p)
    {
        auto g = print_indent("for_statement");
    }
    void operator()(const for_each& p)
    {
        auto g = print_indent("for_each");
    }
    void operator()(const function_definition& p)
    {
        auto g = print_indent("function_definition");

        (*this)(p.body);
    }
    void operator()(const local_function& p)
    {
        auto g = print_indent("local_function");
    }
    void operator()(const local_variables& p)
    {
        auto g = print_indent("local_variables");
        (*this)(p.names);
        (*this)(p.explist);
    }

    void operator()(const expression& p)
    {
        std::visit(*this, p.inner);
    }

    void operator()(const expression_list& p)
    {
        for (auto& exp : p)
        {
            (*this)(exp);
        }
    }

    void operator()(const nil& p)
    {
        print("nil");
    }
    void operator()(const boolean& p)
    {
        print(p.value ? "true" : "false");
    }
    void operator()(const int_type& p)
    {
        print(std::to_string(p));
    }
    void operator()(const float_type& p)
    {
        print(std::to_string(p));
    }
    void operator()(const literal& p)
    {
        print("\"" + p.str + "\"");
    }
    void operator()(const ellipsis& p)
    {
        print("...");
    }
    void operator()(const function_body& p)
    {
        auto g = print_indent("function_body");
        (*this)(p.params);
        if (p.vararg)
            print("...");

        (*this)(p.inner);
    }

    void operator()(const table_constructor& p)
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
    void operator()(const bin_operation& p)
    {
        auto g = print_indent("bin_operation");
        (*this)(p.lhs);
        (*this)(p.rhs);
    }
    void operator()(const un_operation& p)
    {
        auto g = print_indent("un_operation");
        (*this)(p.rhs);
    }

    template<typename T>
    void operator()(const box<T>& p)
    {
        (*this)(*p);
    }

    void operator()(const name_list& p)
    {
        for (auto& n : p)
        {
            print(n);
        }
    }

    void operator()(const block& p)
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
} // namespace ast