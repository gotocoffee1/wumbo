#pragma once

#include "ast.hpp"
#include "parser.hpp"

#include <tao/pegtl/contrib/unescape.hpp>

#include <charconv>

namespace wumbo::lua53
{
using namespace TAO_PEGTL_NAMESPACE;

// clang-format off
template<typename Rule, typename = void>
struct action : nothing<Rule>  {};

// clang-format on

//template<typename Rule>
//struct action<Rule>
//{
//    template<typename ActionInput>
//    static void apply(const ActionInput& in)
//    {
//        std::cout << typeid(Rule).name() << std::endl;
//    }
//};

template<typename State>
struct fill_expression : change_states<State>
{
    template<typename ParseInput>
    static void success(const ParseInput&, State& c, ast::expr_temp& p)
    {
        using s = std::conditional_t<std::is_same_v<State, ast::prefixexp>, box<State>, State>;

        p.list.emplace_back().inner.emplace<s>(std::move(c));
    }
};

template<auto ParseMode, size_t Prefix>
struct parse_num
{
    template<typename ActionInput, typename T>
    static bool apply(const ActionInput& in, T& num)
    {
        auto sv = in.string_view();
        sv.remove_prefix(Prefix);
#ifndef __clang__
        auto [ptr, ec] = std::from_chars(sv.data(), sv.data() + sv.size(), num, ParseMode);
        return ec == std::errc{};
#else
        return true;
#endif //  __clang__
    }
};
// clang-format off
template<> struct action<hexadecimal> : parse_num<std::chars_format::hex, 2>{};

template<> struct action<decimal> : parse_num<std::chars_format::general, 0>{};

template<> struct action<hexinteger> : parse_num<16, 2>{};

template<> struct action<decinteger> : parse_num<10, 0>{};

template<> struct action<floatnumeral> : fill_expression<ast::float_type>{};

template<> struct action<intnumeral> : fill_expression<ast::int_type>{};

template<> struct action<key_nil> : fill_expression<ast::nil>{};

// clang-format on
template<>
struct action<ellipsis> : fill_expression<ast::ellipsis>
{
    template<typename ParseInput>
    static void success(const ParseInput&, ast::ellipsis&, ast::function_body& p)
    {
        p.vararg = true;
    }
    using fill_expression::success;
};

template<>
struct action<key_true> : fill_expression<ast::boolean>
{
    static void apply0(ast::boolean& p)
    {
        p.value = true;
    }
};

template<>
struct action<key_false> : fill_expression<ast::boolean>
{
    static void apply0(ast::boolean& p)
    {
        p.value = false;
    }
};
// clang-format off

template<>struct action<regular> : unescape::append_all {};
template<>struct action<hexbyte> : unescape::unescape_x{};
template<> struct action<single> : unescape::unescape_c<single, '\a', '\b', '\f', '\n', '\r', '\t', '\v', '\\', '"', '\'', '\0', '\n'>{};

// clang-format on

template<char Q>
struct action<short_string<Q>> : change_states<std::string>
{
    template<typename ParseInput>
    static void success(const ParseInput&, std::string& c, ast::literal& p)
    {
        p.str = std::move(c);
    }
};

template<>
struct action<typename long_string::content>
{
    template<typename ActionInput>
    static void apply(const ActionInput& in, ast::literal& p)
    {
        p.str = in.string();
    }

    template<typename ActionInput, typename T>
    static void apply(const ActionInput& in, T&, ast::literal& p)
    {
        p.str = in.string();
    }
};

template<>
struct action<literal_string> : fill_expression<ast::literal>
{
    template<typename ParseInput>
    static void success(const ParseInput&, ast::literal& c, ast::arg_list& p)
    {
        p.emplace_back().inner.emplace<ast::literal>(std::move(c));
    }
    using fill_expression::success;
};

template<>
struct action<name>
    : change_states<ast::name_t>
{
    template<typename ActionInput>
    static void apply(const ActionInput& in, ast::name_t& name)
    {
        name = in.string();
    }

    template<typename ParseInput>
    static void success(const ParseInput&, ast::name_t& name, ast::goto_statement& p)
    {
        p.name = std::move(name);
    }

    template<typename ParseInput>
    static void success(const ParseInput&, ast::name_t& name, ast::label_statement& p)
    {
        p.name = std::move(name);
    }

    template<typename ParseInput>
    static void success(const ParseInput&, ast::name_t& name, ast::local_function& p)
    {
        p.name = std::move(name);
    }

    template<typename ParseInput>
    static void success(const ParseInput&, ast::name_t& c, ast::for_combined& p)
    {
        p.first = std::move(c);
    }

    template<typename ParseInput>
    static void success(const ParseInput&, ast::name_t& name, ast::name_list& p)
    {
        p.push_back(std::move(name));
    }

    template<typename ParseInput>
    static void success(const ParseInput&, ast::name_t& name, std::pair<ast::name_t, ast::expression>& p)
    {
        p.first = std::move(name);
    }

    template<typename ParseInput>
    static void success(const ParseInput&, ast::name_t& name, ast::vartail& p)
    {
        p.emplace<ast::name_t>(std::move(name));
    }

    template<typename ParseInput>
    static void success(const ParseInput&, ast::name_t& name, ast::varhead& p)
    {
        p.emplace<ast::name_t>(std::move(name));
    }

    template<typename ParseInput>
    static void success(const ParseInput&, ast::name_t& name, ast::functail& p)
    {
        p.name.emplace(std::move(name));
    }

    template<typename ParseInput>
    static void success(const ParseInput&, ast::name_t& name, ast::funchead& p)
    {
        p.emplace<ast::name_t>(std::move(name));
    }

    template<typename ParseInput>
    static void success(const ParseInput&, ast::name_t& name, ast::prefixexp& p)
    {
        p.chead.emplace<ast::name_t>(std::move(name));
    }

    template<typename ParseInput>
    static void success(const ParseInput&, ast::name_t& name, std::pair<ast::function_definition, bool>& p)
    {
        p.first.function_name.push_back(std::move(name));
    }
};

template<>
struct action<name_list>
    : change_states<ast::name_list>
{
    template<typename ParseInput>
    static void success(const ParseInput&, ast::name_list& c, ast::function_body& p)
    {
        p.params = std::move(c);
    }
};

template<>
struct action<name_list_must>
    : change_states<ast::name_list>
{
    template<typename ParseInput>
    static void success(const ParseInput&, ast::name_list& c, ast::for_each& p)
    {
        p.names = std::move(c);
    }

    template<typename ParseInput>
    static void success(const ParseInput&, ast::name_list& c, ast::local_variables& p)
    {
        p.names = std::move(c);
    }
};

template<>
struct action<function_call_tail> : change_states<ast::functail>
{
    template<typename ParseInput>
    static void success(const ParseInput&, ast::functail& c, ast::function_call::tail_t& p)
    {
        p.second = std::move(c);
    }

    template<typename ParseInput>
    static void success(const ParseInput&, ast::functail& c, ast::var::tail_t& p)
    {
        p.first.push_back(std::move(c));
    }

    template<typename ParseInput>
    static void success(const ParseInput&, ast::functail& c, ast::prefixexp& p)
    {
        p.tail.emplace_back(std::move(c));
    }
};

template<>
struct action<variable_tail> : change_states<ast::vartail>
{
    template<typename ParseInput>
    static void success(const ParseInput&, ast::vartail& c, ast::var::tail_t& p)
    {
        p.second = std::move(c);
    }

    template<typename ParseInput>
    static void success(const ParseInput&, ast::vartail& c, ast::function_call::tail_t& p)
    {
        p.first.push_back(std::move(c));
    }

    template<typename ParseInput>
    static void success(const ParseInput&, ast::vartail& c, ast::prefixexp& p)
    {
        p.tail.emplace_back(std::move(c));
    }

    template<typename ParseInput>
    static void success(const ParseInput&, ast::vartail& c, std::pair<ast::expression, ast::vartail>& p)
    {
        p.second = std::move(c);
    }
};

template<>
struct action<function_call_head> : change_states<ast::funchead>
{
    template<typename ParseInput>
    static void success(const ParseInput&, ast::funchead& c, ast::function_call& p)
    {
        p.head = std::move(c);
    }
};

template<>
struct action<variable_head_one> : change_states<std::pair<ast::expression, ast::vartail>>
{
    template<typename ParseInput>
    static void success(const ParseInput&, std::pair<ast::expression, ast::vartail>& c, ast::varhead& p)
    {
        p.emplace<std::pair<ast::expression, ast::vartail>>(std::move(c));
    }
};

template<>
struct action<variable_head> : change_states<ast::varhead>
{
    template<typename ParseInput>
    static void success(const ParseInput&, ast::varhead& c, ast::var& p)
    {
        p.head = std::move(c);
    }
};

template<>
struct action<expr_thirteen> : fill_expression<ast::prefixexp>
{
};

template<>
struct action<function_call_one> : change_states<ast::function_call::tail_t>
{
    template<typename ParseInput>
    static void success(const ParseInput&, ast::function_call::tail_t& c, ast::function_call& p)
    {
        p.tail.push_back(std::move(c));
    }
};

template<>
struct action<variable_one> : change_states<ast::var::tail_t>
{
    template<typename ParseInput>
    static void success(const ParseInput&, ast::var::tail_t& c, ast::var& p)
    {
        p.tail.push_back(std::move(c));
    }
};

template<>
struct action<variable> : change_states<ast::var>
{
    template<typename ParseInput>
    static void success(const ParseInput&, ast::var& c, ast::assignments& p)
    {
        p.varlist.push_back(std::move(c));
    }
};

template<>
struct action<method_name>
{
    static void apply0(std::pair<ast::function_definition, bool>& p)
    {
        p.second = true;
    }
};

template<>
struct action<function_args> : change_states<ast::arg_list>
{
    template<typename ParseInput>
    static void success(const ParseInput&, ast::arg_list& c, ast::functail& p)
    {
        p.args = std::move(c);
    }
};

template<>
struct action<function_body> : fill_expression<ast::function_body>
{
    template<typename ParseInput>
    static void success(const ParseInput&, ast::function_body& c, std::pair<ast::function_definition, bool>& p)
    {
        p.first.body = std::move(c);
    }

    template<typename ParseInput>
    static void success(const ParseInput&, ast::function_body& c, ast::local_function& p)
    {
        p.body = std::move(c);
    }
    using fill_expression::success;
};

template<>
struct action<expr_list_must> : change_states<ast::expression_list>
{
    template<typename ParseInput>
    static void success(const ParseInput&, ast::expression_list& c, ast::assignments& p)
    {
        p.explist = std::move(c);
    }

    template<typename ParseInput>
    static void success(const ParseInput&, ast::expression_list& c, ast::local_variables& p)
    {
        p.explist = std::move(c);
    }

    template<typename ParseInput>
    static void success(const ParseInput&, ast::expression_list& c, ast::for_each& p)
    {
        p.explist = std::move(c);
    }

    template<typename ParseInput>
    static void success(const ParseInput&, ast::expression_list& c, ast::return_stat& p)
    {
        p.emplace(std::move(c));
    }

    template<typename ParseInput>
    static void success(const ParseInput&, ast::expression_list& c, ast::arg_list& p)
    {
        p = std::move(c);
    }
};

template<>
struct action<expression> : change_states<ast::expression>
{
    template<typename ParseInput>
    static void success(const ParseInput&, ast::expression& c, ast::expression_list& p)
    {
        p.push_back(std::move(c));
    }

    template<typename ParseInput>
    static void success(const ParseInput&, ast::expression& c, ast::while_statement& p)
    {
        p.condition = std::move(c);
    }

    template<typename ParseInput>
    static void success(const ParseInput&, ast::expression& c, ast::repeat_statement& p)
    {
        p.condition = std::move(c);
    }

    template<typename ParseInput>
    static void success(const ParseInput&, ast::expression& c, ast::if_statement& p)
    {
        p.cond_block.emplace_back(std::move(c), ast::block{});
    }

    template<typename ParseInput>
    static void success(const ParseInput&, ast::expression& c, std::pair<ast::name_t, ast::expression>& p)
    {
        p.second = std::move(c);
    }

    template<typename ParseInput>
    static void success(const ParseInput&, ast::expression& c, std::pair<std::optional<ast::expression>, ast::expression>& p)
    {
        if (p.first)
            p.second = std::move(c);
        else
            p.first = std::move(c);
    }

    template<typename ParseInput>
    static void success(const ParseInput&, ast::expression& c, ast::field& p)
    {
        p.index.emplace<std::monostate>();
        p.value = std::move(c);
    }

    template<typename ParseInput>
    static void success(const ParseInput&, ast::expression& c, ast::vartail& p)
    {
        p.emplace<ast::expression>(std::move(c));
    }

    template<typename ParseInput>
    static void success(const ParseInput&, ast::expression& c, ast::funchead& p)
    {
        p.emplace<ast::expression>(std::move(c));
    }

    template<typename ParseInput>
    static void success(const ParseInput&, ast::expression& c, std::pair<ast::expression, ast::vartail>& p)
    {
        p.first = std::move(c);
    }

    template<typename ParseInput>
    static void success(const ParseInput&, ast::expression& c, ast::prefixexp& p)
    {
        p.chead.emplace<ast::expression>(std::move(c));
    }

    template<typename ParseInput>
    static void success(const ParseInput&, ast::expression& c, ast::for_statement& p)
    {
        p.exp.push_back(std::move(c));
    }
};

struct operation : change_state<ast::expr_temp>
{
    static void recursive(ast::expr_temp& c, ast::expression& p)
    {
        if (c.list.size() == 1)
        {
            if (c.u_op)
            {
                auto& temp = *p.inner.emplace<box<ast::un_operation>>();
                temp.op    = *c.u_op;
                temp.rhs   = std::move(c.list[0]);
            }
            else
                p = std::move(c.list[0]);
        }
        else if (c.list.size() == 2)
        {
            auto& temp = *p.inner.emplace<box<ast::bin_operation>>();
            temp.op    = c.b_op[0];
            temp.lhs   = std::move(c.list[0]);
            temp.rhs   = std::move(c.list[1]);
        }
        else
        {
            auto& temp = *p.inner.emplace<box<ast::bin_operation>>();
            temp.op    = c.b_op.back();
            temp.rhs   = std::move(c.list.back());
            c.list.pop_back();
            c.b_op.pop_back();
            recursive(c, temp.lhs);
        }
    }

    template<typename ParseInput>
    static void success(const ParseInput&, ast::expr_temp& c, ast::expression& p)
    {
        recursive(c, p);
    }

    template<typename ParseInput>
    static void success(const ParseInput&, ast::expr_temp& c, ast::expr_temp& p)
    {
        if (c.list.size() == 1 && p.list.empty() && !p.u_op) // optimize
            p = std::move(c);
        else
            recursive(c, p.list.emplace_back());
    }
};

template<ast::bin_operator Op>
struct op
{
    static void apply0(ast::expr_temp& p)
    {
        p.b_op.push_back(Op);
    }
};

template<ast::un_operator Op>
struct un_op
{
    static void apply0(ast::un_operator& p)
    {
        p = Op;
    }
};

template<ast::bin_operator B, ast::un_operator U>
struct both_op
    : op<B>
    , un_op<U>
{
    using op<B>::apply0;
    using un_op<U>::apply0;
};

template<>
struct action<unary_operators> : change_states<ast::un_operator>
{
    template<typename ParseInput>
    static void success(const ParseInput&, ast::un_operator& c, ast::expr_temp& p)
    {
        p.u_op = c;
    }
};

// clang-format off
template<> struct action<expr_zero> : operation{};
template<> struct action<expr_one> : operation{};
template<> struct action<expr_two> : operation{};
template<> struct action<expr_three> : operation{};
template<> struct action<expr_four> : operation{};
template<> struct action<expr_five> : operation{};
template<> struct action<expr_six> : operation{};
template<> struct action<expr_seven> : operation{};
template<> struct action<expr_eight> : operation{};
template<> struct action<expr_nine> : operation{};
template<> struct action<expr_ten> : operation{};
template<> struct action<expr_eleven> : operation{};

template<> struct action<key_or> : op<ast::bin_operator::logic_or>{};
template<> struct action<key_and> : op<ast::bin_operator::logic_and>{};
template<> struct action<op_plus> : op<ast::bin_operator::addition>{};
template<> struct action<op_minus> : both_op<ast::bin_operator::subtraction, ast::un_operator::minus>{};
template<> struct action<op_multiply> : op<ast::bin_operator::multiplication>{};
template<> struct action<op_modulo> : op<ast::bin_operator::modulo>{};
template<> struct action<op_divide> : op<ast::bin_operator::division>{};
template<> struct action<op_divide_floor> : op<ast::bin_operator::division_floor>{};
template<> struct action<op_or> : op<ast::bin_operator::binary_or>{};
template<> struct action<op_and> : op<ast::bin_operator::binary_and>{};
template<> struct action<op_lshift> : op<ast::bin_operator::binary_left_shift>{};
template<> struct action<op_rshift> : op<ast::bin_operator::binary_right_shift>{};
template<> struct action<op_power> : op<ast::bin_operator::exponentiation>{};
template<> struct action<op_eq> : op<ast::bin_operator::equality>{};
template<> struct action<op_lteq> : op<ast::bin_operator::less_or_equal>{};
template<> struct action<op_gteq> : op<ast::bin_operator::greater_or_equal>{};
template<> struct action<op_lt> : op<ast::bin_operator::less_than>{};
template<> struct action<op_gt> : op<ast::bin_operator::greater_than>{};
template<> struct action<op_neq> : op<ast::bin_operator::inequality>{};
template<> struct action<op_concat> : op<ast::bin_operator::concat>{};
template<> struct action<op_tilde> : both_op<ast::bin_operator::binary_xor, ast::un_operator::binary_not>{};
template<> struct action<op_len> : un_op<ast::un_operator::len>{};
template<> struct action<key_not> : un_op<ast::un_operator::logic_not>{};
// clang-format on

template<>
struct action<statement_return> : change_states<ast::return_stat>
{
    template<typename ParseInput>
    static void success(const ParseInput&, ast::return_stat& c, ast::block& p)
    {
        if (!c)
            p.retstat.emplace();
        else
            p.retstat = std::move(c);
    }
};

template<typename T>
struct state : change_states<T>
{
    template<typename ParseInput>
    static void success(const ParseInput&, T& c, ast::statement& p)
    {
        p.inner.emplace<T>(std::move(c));
    }
};
// clang-format off
template<> struct action<assignments> : state<ast::assignments>{};

template<> struct action<function_call> : state<ast::function_call>{};

template<> struct action<key_break> : state<ast::key_break>{};

template<> struct action<goto_statement> : state<ast::goto_statement>{};

template<> struct action<label_statement> : state<ast::label_statement>{};

template<> struct action<do_statement> : state<ast::do_statement>{};

template<> struct action<while_statement> : state<ast::while_statement>{};

template<> struct action<repeat_statement> : state<ast::repeat_statement>{};

template<> struct action<if_statement> : state<ast::if_statement>{};

template<> struct action<local_function> : state<ast::local_function>{};

template<> struct action<local_variables> : state<ast::local_variables>{};
// clang-format on

template<>
struct action<else_statement> : change_states<ast::block>
{
    template<typename ParseInput>
    static void success(const ParseInput&, ast::block& c, ast::if_statement& p)
    {
        p.else_block.emplace(std::move(c));
    }
};

template<>
struct action<function_definition> : change_states<std::pair<ast::function_definition, bool>>
{
    template<typename ParseInput>
    static void success(const ParseInput&, std::pair<ast::function_definition, bool>& c, ast::statement& p)
    {
        auto& [def, is_method] = c;
        if (is_method)
        {
            def.body.params.insert(def.body.params.begin(), "self");
        }
        p.inner.emplace<ast::function_definition>(std::move(def));
    }
};

template<>
struct action<for_statement_one> : change_states<ast::for_statement>
{
    template<typename ParseInput>
    static void success(const ParseInput&, ast::for_statement& c, ast::for_combined& p)
    {
        p.condition.emplace<ast::for_statement>(std::move(c));
    }
};

template<>
struct action<for_statement_two> : change_states<ast::for_each>
{
    template<typename ParseInput>
    static void success(const ParseInput&, ast::for_each& c, ast::for_combined& p)
    {
        p.condition.emplace<ast::for_each>(std::move(c));
    }
};

template<>
struct action<for_statement> : change_states<ast::for_combined>
{
    template<typename ParseInput>
    static void success(const ParseInput&, ast::for_combined& c, ast::statement& p)
    {
        if (auto f = std::get_if<ast::for_each>(&c.condition); f)
        {
            auto& temp = p.inner.emplace<ast::for_each>(std::move(*f));
            temp.inner = std::move(c.inner);
            temp.names.insert(temp.names.begin(), std::move(c.first));
        }
        else if (auto fs = std::get_if<ast::for_statement>(&c.condition); fs)
        {
            auto& temp = p.inner.emplace<ast::for_statement>(std::move(*fs));
            temp.inner = std::move(c.inner);
            temp.var   = std::move(c.first);
        }
    }
};

template<>
struct action<table_field_two> : change_states<std::pair<ast::name_t, ast::expression>>
{
    template<typename ParseInput>
    static void success(const ParseInput&, std::pair<ast::name_t, ast::expression>& c, ast::field& p)
    {
        p.index = std::move(c.first);
        p.value = std::move(c.second);
    }
};

template<>
struct action<table_field_one> : change_states<std::pair<std::optional<ast::expression>, ast::expression>>
{
    template<typename ParseInput>
    static void success(const ParseInput&, std::pair<std::optional<ast::expression>, ast::expression>& c, ast::field& p)
    {
        p.index = std::move(*c.first);
        p.value = std::move(c.second);
    }
};

template<>
struct action<table_field> : change_states<ast::field>
{
    template<typename ParseInput>
    static void success(const ParseInput&, ast::field& c, ast::table_constructor& p)
    {
        p.push_back(std::move(c));
    }
};

template<>
struct action<table_constructor> : fill_expression<ast::table_constructor>
{
    template<typename ParseInput>
    static void success(const ParseInput&, ast::table_constructor& c, ast::arg_list& p)
    {
        p.emplace_back().inner.emplace<ast::table_constructor>(std::move(c));
    }

    using fill_expression::success;
};

template<>
struct action<statement> : change_states<ast::statement>
{
    template<typename ParseInput>
    static void success(const ParseInput&, ast::statement& c, ast::block& p)
    {
        p.statements.push_back(std::move(c));
    }
};

template<typename End>
struct action<statement_list<End>> : change_states<ast::block>
{
    template<typename ParseInput, typename T>
    static void success(const ParseInput&, ast::block& c, T& p)
    {
        p.inner = std::move(c);
    }

    template<typename ParseInput>
    static void success(const ParseInput&, ast::block& c, ast::block& p)
    {
        p = std::move(c);
    }

    template<typename ParseInput>
    static void success(const ParseInput&, ast::block& c, ast::if_statement& p)
    {
        p.cond_block.back().second = std::move(c);
    }
};

} // namespace lua53
