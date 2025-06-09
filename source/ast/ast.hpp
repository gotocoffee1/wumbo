#pragma once

#include "utils/box.hpp"
#include "utils/type.hpp"
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace wumbo::ast
{

struct local_usage
{
    size_t write_count = 0;
    size_t read_count  = 0;
    bool init          = false;
    bool upvalue       = false;

    bool is_upvalue() const
    {
        return upvalue && write_count > 0;
    }
};

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
using int_type   = int64_t;

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
    std::vector<local_usage> usage;
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
    local_usage usage;
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

using arg_list = expression_list; // size >=0

//   vartail ::= '[' exp ']' | '.' Name
using vartail = std::variant<expression, name_t>;
//   varhead ::= Name | '(' exp ')' vartail
using varhead = std::variant<name_t, std::pair<expression, vartail>>;
//   functail ::= args | ':' Name args
struct functail
{
    std::optional<name_t> name;
    arg_list args;
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
    std::vector<local_usage> usage;
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
