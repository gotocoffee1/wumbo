// Copyright (c) 2015-2023 Dr. Colin Hirsch and Daniel Frey
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#ifndef TAO_PEGTL_SRC_EXAMPLES_PEGTL_LUA53_HPP
#define TAO_PEGTL_SRC_EXAMPLES_PEGTL_LUA53_HPP

#if !defined(__cpp_exceptions)
#error "Exception support required for parser.hpp"
#else

#include <tao/pegtl.hpp>
#include <tao/pegtl/contrib/raw_string.hpp>

namespace wumbo::lua53
{
using namespace TAO_PEGTL_NAMESPACE;

// PEGTL grammar for the Lua 5.3.0 lexer and parser.
//
// The grammar here is not very similar to the grammar
// in the Lua reference documentation on which it is based
// which is due to multiple causes.
//
// The main difference is that this grammar includes really
// "everything", not just the structural parts from the
// reference documentation:
// - The PEG-approach combines lexer and parser; this grammar
//   handles comments and tokenisation.
// - The operator precedence and associativity are reflected
//   in the structure of this grammar.
// - All details for all types of literals are included, with
//   escape-sequences for literal strings, and long literals.
//
// The second necessary difference is that all left-recursion
// had to be eliminated.
//
// In some places the grammar was optimised to require as little
// back-tracking as possible, most prominently for expressions.
// The original grammar contains the following production rules:
//
//   prefixexp ::= var | functioncall | '(' exp ')'
//   functioncall ::=  prefixexp args | prefixexp ':' Name args
//   var ::=  Name | prefixexp '[' exp ']' | prefixexp '.' Name
//
// We need to eliminate the left-recursion, and we also want to
// remove the ambiguity between function calls and variables,
// i.e. the fact that we can have expressions like
//
//   ( a * b ).c()[ d ].e:f()
//
// where only the last element decides between function call and
// variable, making it necessary to parse the whole thing again
// if we chose wrong at the beginning.
// First we eliminate prefixexp and obtain:
//
//   functioncall ::=  ( var | functioncall | '(' exp ')' ) ( args | ':' Name args )
//   var ::=  Name | ( var | functioncall | '(' exp ')' ) ( '[' exp ']' | '.' Name )
//
// Next we split function_call and variable into a first part,
// a "head", or how they can start, and a second part, the "tail",
// which, in a sequence like above, is the final deciding part:
//
//   vartail ::= '[' exp ']' | '.' Name
//   varhead ::= Name | '(' exp ')' vartail
//   functail ::= args | ':' Name args
//   funchead ::= Name | '(' exp ')'
//
// This allows us to rewrite var and function_call as follows.
//
//   var ::= varhead { { functail } vartail }
//   function_call ::= funchead [ { vartail } functail ]
//
// Finally we can define a single expression that takes care
// of var, function_call, and expressions in a bracket:
//
//   chead ::= '(' exp ')' | Name
//   combined ::= chead { functail | vartail }
//
// Such a combined expression starts with a bracketed
// expression or a name, and continues with an arbitrary
// number of functail and/or vartail parts, all in a one
// grammar rule without back-tracking.
//
// The rule expr_thirteen below implements "combined".
//
// Another issue of interest when writing a PEG is how to
// manage the separators, the white-space and comments that
// can occur in many places; in the classical two-stage
// lexer-parser approach the lexer would have taken care of
// this, but here we use the PEG approach that combines both.
//
// In the following grammar most rules adopt the convention
// that they take care of "internal padding", i.e. spaces
// and comments that can occur within the rule, but not
// "external padding", i.e. they don't start or end with
// a rule that "eats up" all extra padding (spaces and
// comments). In some places, where it is more efficient,
// right padding is used.

// clang-format off
   struct short_comment : until< eolf > {};
   struct long_string : raw_string< '[', '=', ']'> {};
   struct comment : disable< two< '-' >, sor< long_string, short_comment > > {};

   struct sep : sor< ascii::space, comment > {};
   struct seps : star< sep > {};

   struct str_and : TAO_PEGTL_STRING( "and" ) {};
   struct str_break : TAO_PEGTL_STRING( "break" ) {};
   struct str_do : TAO_PEGTL_STRING( "do" ) {};
   struct str_else : TAO_PEGTL_STRING( "else" ) {};
   struct str_elseif : TAO_PEGTL_STRING( "elseif" ) {};
   struct str_end : TAO_PEGTL_STRING( "end" ) {};
   struct str_false : TAO_PEGTL_STRING( "false" ) {};
   struct str_for : TAO_PEGTL_STRING( "for" ) {};
   struct str_function : TAO_PEGTL_STRING( "function" ) {};
   struct str_goto : TAO_PEGTL_STRING( "goto" ) {};
   struct str_if : TAO_PEGTL_STRING( "if" ) {};
   struct str_in : TAO_PEGTL_STRING( "in" ) {};
   struct str_local : TAO_PEGTL_STRING( "local" ) {};
   struct str_nil : TAO_PEGTL_STRING( "nil" ) {};
   struct str_not : TAO_PEGTL_STRING( "not" ) {};
   struct str_or : TAO_PEGTL_STRING( "or" ) {};
   struct str_repeat : TAO_PEGTL_STRING( "repeat" ) {};
   struct str_return : TAO_PEGTL_STRING( "return" ) {};
   struct str_then : TAO_PEGTL_STRING( "then" ) {};
   struct str_true : TAO_PEGTL_STRING( "true" ) {};
   struct str_until : TAO_PEGTL_STRING( "until" ) {};
   struct str_while : TAO_PEGTL_STRING( "while" ) {};

   // Note that 'elseif' precedes 'else' in order to prevent only matching
   // the "else" part of an "elseif" and running into an error in the
   // 'keyword' rule.

   template< typename Key >
   struct key : seq< Key, not_at< identifier_other > > {};

   struct sor_keyword : sor< str_and, str_break, str_do, str_elseif, str_else, str_end, str_false, str_for, str_function, str_goto, str_if, str_in, str_local, str_nil, str_not, str_repeat, str_return, str_then, str_true, str_until, str_while > {};

   struct key_and : key< str_and > {};
   struct key_break : key< str_break > {};
   struct key_do : key< str_do > {};
   struct key_else : key< str_else > {};
   struct key_elseif : key< str_elseif > {};
   struct key_end : key< str_end > {};
   struct key_false : key< str_false > {};
   struct key_for : key< str_for > {};
   struct key_function : key< str_function > {};
   struct key_goto : key< str_goto > {};
   struct key_if : key< str_if > {};
   struct key_in : key< str_in > {};
   struct key_local : key< str_local > {};
   struct key_nil : key< str_nil > {};
   struct key_not : key< str_not > {};
   struct key_or : key< str_or > {};
   struct key_repeat : key< str_repeat > {};
   struct key_return : key< str_return > {};
   struct key_then : key< str_then > {};
   struct key_true : key< str_true > {};
   struct key_until : key< str_until > {};
   struct key_while : key< str_while > {};

   struct keyword : key< sor_keyword > {};

   template< typename R >
   struct l_pad : pad< R, sep > {};

   struct name : seq< not_at< keyword >, identifier > {};

   struct single : one< 'a', 'b', 'f', 'n', 'r', 't', 'v', '\\', '"', '\'', '0', '\n' > {};
   struct spaces : seq< one< 'z' >, star< space > > {};
   struct hexbyte : if_must< one< 'x' >, xdigit, xdigit > {};
   struct decbyte : if_must< digit, rep_opt< 2, digit > > {};
   struct unichar : if_must< one< 'u' >, one< '{' >, plus< xdigit >, one< '}' > > {};
   struct escaped : if_must< one< '\\' >, sor< hexbyte, decbyte, unichar, single, spaces > > {};
   struct regular : not_one< '\r', '\n' > {};
   struct character : sor< escaped, regular > {};

   template< char Q >
   struct short_string : if_must< one< Q >, until< one< Q >, character > > {};
   struct literal_string : sor< short_string< '"' >, short_string< '\'' >, long_string > {};

   template< typename E >
   struct exponent : opt_must< E, opt< one< '+', '-' > >, plus< digit > > {};

   template< typename D, typename E >
   struct numeral_three : seq< if_must< one< '.' >, plus< D > >, exponent< E > > {};
   template< typename D, typename E >
   struct numeral_two : seq< plus< D >, opt< one< '.' >, star< D > >, exponent< E > > {};
   template< typename D, typename E >
   struct numeral_one : sor< numeral_two< D, E >, numeral_three< D, E > > {};

   struct decimal : numeral_one< digit, one< 'e', 'E' > > {};
   struct hexadecimal : if_must< istring< '0', 'x' >, numeral_one< xdigit, one< 'p', 'P' > > > {};
   struct decinteger : seq<plus< digit >,  not_at<one<'.'>>> {};
   struct hexinteger : if_must< istring< '0', 'x' >, plus< xdigit >, not_at<one<'.'>> > {};

   struct intnumeral : sor<hexinteger, decinteger> {};
   struct floatnumeral : sor<hexadecimal, decimal> {};

   struct numeral : sor<intnumeral, floatnumeral > {};

   struct label_statement : if_must< two< ':' >, seps, name, seps, two< ':' > > {};
   struct goto_statement : if_must< key_goto, seps, name > {};

   struct statement;
   struct expression;

   struct name_list : list< name, one< ',' >, sep > {};
   struct name_list_must : list_must< name, one< ',' >, sep > {};
   struct expr_list_must : list_must< expression, one< ',' >, sep > {};

   struct statement_return : seq< pad_opt< expr_list_must, sep >, opt< one< ';' >, seps > > {};

   template< typename E >
   struct statement_list : seq< seps, until< sor< E, if_must< key_return, statement_return, E > >, statement, seps > > {};

   template< char O, char... N >
   struct op_one : seq< one< O >, at< not_one< N... > > > {};
   template< char O, char P, char... N >
   struct op_two : seq< string< O, P >, at< not_one< N... > > > {};

   struct table_field_one : if_must< one< '[' >, seps, expression, seps, one< ']' >, seps, one< '=' >, seps, expression > {};
   struct table_field_two : if_must< seq< name, seps, op_one< '=', '=' > >, seps, expression > {};
   struct table_field : sor< table_field_one, table_field_two, expression > {};
   struct table_field_list : list_tail< table_field, one< ',', ';' >, sep > {};
   struct table_constructor : if_must< one< '{' >, pad_opt< table_field_list, sep >, one< '}' > > {};

   struct parameter_list_one : seq< name_list, opt_must< l_pad< one< ',' > >, ellipsis > > {};
   struct parameter_list : sor< ellipsis, parameter_list_one > {};

   struct function_body : seq< one< '(' >, pad_opt< parameter_list, sep >, one< ')' >, seps, statement_list< key_end > > {};
   struct function_literal : if_must< key_function, seps, function_body > {};

   struct bracket_expr : if_must< one< '(' >, seps, expression, seps, one< ')' > > {};

   struct function_args_one : if_must< one< '(' >, pad_opt< expr_list_must, sep >, one< ')' > > {};
   struct function_args : sor< function_args_one, table_constructor, literal_string > {};

   struct variable_tail_one : if_must< one< '[' >, seps, expression, seps, one< ']' > > {};
   struct variable_tail_two : if_must< seq< not_at< two< '.' > >, one< '.' > >, seps, name > {};
   struct variable_tail : sor< variable_tail_one, variable_tail_two > {};

   struct function_call_tail_one : if_must< seq< not_at< two< ':' > >, one< ':' > >, seps, name, seps, function_args > {};
   struct function_call_tail : sor< function_args, function_call_tail_one > {};

   struct variable_head_one : seq< bracket_expr, seps, variable_tail > {};
   struct variable_head : sor< name, variable_head_one > {};

   struct function_call_head : sor< name, bracket_expr > {};

   struct variable_one : seq< star< seps, function_call_tail >, seps, variable_tail > {};
   struct variable : seq< variable_head, star< variable_one > > {};
   struct function_call_one : until< seq< seps, function_call_tail >, seps, variable_tail > {};
   struct function_call : seq< function_call_head, plus< function_call_one > > {};

   template< typename S, typename O >
   struct left_assoc : seq< S, seps, star_must< O, seps, S, seps > > {};
   template< typename S, typename O >
   struct right_assoc : seq< S, seps, opt_must< O, seps, right_assoc< S, O > > > {};

   struct op_modulo : one< '%' > {};
   struct op_divide : one< '/' > {};
   struct op_divide_floor : two< '/' > {};
   struct op_multiply : one< '*' > {};
   struct op_plus : one< '+' > {};
   struct op_minus : one< '-' > {};
   struct op_len : one< '#' > {};
   struct op_tilde : op_one< '~', '=' > {};
   struct op_lshift : two< '<' > {};
   struct op_rshift : two< '>' > {};
   struct op_power : one< '^' > {};
   struct op_or : one< '|' > {};
   struct op_and : one< '&' > {};
   struct op_concat : op_two< '.', '.', '.' > {};
   struct op_eq : two< '=' > {};
   struct op_lteq : string< '<', '=' > {};
   struct op_gteq : string< '>', '=' > {};
   struct op_lt : op_one< '<', '<' > {};
   struct op_gt : op_one< '>', '>' > {};
   struct op_neq : string< '~', '=' > {};

   struct unary_operators : sor< op_minus,
                                        op_len,
                                        op_tilde,
                                        key_not > {};

   struct expr_ten;
   struct expr_thirteen : seq< sor< bracket_expr, name >, star< seps, sor< function_call_tail, variable_tail > > > {};
   struct expr_twelve : sor< key_nil,
                                    key_true,
                                    key_false,
                                    ellipsis,
                                    numeral,
                                    literal_string,
                                    function_literal,
                                    expr_thirteen,
                                    table_constructor > {};
   struct expr_eleven : seq< expr_twelve, seps, opt<op_power, seps, expr_ten, seps > > {};
   struct unary_apply : if_must< unary_operators, seps, expr_ten, seps > {};
   struct expr_ten : sor< unary_apply, expr_eleven > {};
   struct operators_nine : sor< op_divide_floor,
                                op_divide,
                                op_multiply,
                                op_modulo > {};
   struct expr_nine : left_assoc< expr_ten, operators_nine > {};
   struct operators_eight : sor< op_plus, op_minus > {};
   struct expr_eight : left_assoc< expr_nine, operators_eight > {};
   struct expr_seven : right_assoc< expr_eight, op_concat > {};
   struct operators_six : sor< op_lshift, op_rshift > {};
   struct expr_six : left_assoc< expr_seven, operators_six > {};
   struct expr_five : left_assoc< expr_six, op_and > {};
   struct expr_four : left_assoc< expr_five, op_tilde > {};
   struct expr_three : left_assoc< expr_four, op_or > {};
   struct operators_two : sor<  op_eq,
                                op_lteq,
                                op_gteq,
                                op_lt,
                                op_gt,
                                op_neq > {};
   struct expr_two : left_assoc< expr_three, operators_two > {};
   struct expr_one : left_assoc< expr_two, key_and > {};
   struct expr_zero : left_assoc< expr_one, key_or > {};
   struct expression : seq<expr_zero> {};

   struct do_statement : if_must< key_do, statement_list< key_end > > {};
   struct while_statement : if_must< key_while, seps, expression, seps, key_do, statement_list< key_end > > {};
   struct repeat_statement : if_must< key_repeat, statement_list< key_until >, seps, expression > {};

   struct at_elseif_else_end : sor< at< key_elseif >, at< key_else >, at< key_end > > {};
   struct elseif_statement : if_must< key_elseif, seps, expression, seps, key_then, statement_list< at_elseif_else_end > > {};
   struct else_statement : if_must< key_else, statement_list< key_end > > {};
   struct if_statement : if_must< key_if, seps, expression, seps, key_then, statement_list< at_elseif_else_end >, seps, until< sor< else_statement, key_end >, elseif_statement, seps > > {};

   struct for_statement_one : seq< one< '=' >, seps, expression, seps, one< ',' >, seps, expression, pad_opt< if_must< one< ',' >, seps, expression >, sep > > {};
   struct for_statement_two : seq< opt_must< one< ',' >, seps, name_list_must, seps >, key_in, seps, expr_list_must, seps > {};
   struct for_statement : if_must< key_for, seps, name, seps, sor< for_statement_one, for_statement_two >, key_do, statement_list< key_end > > {};

   struct assignment_variable_list : list_must< variable, one< ',' >, sep > {};
   struct assignments_one : if_must< one< '=' >, seps, expr_list_must > {};
   struct assignments : seq< assignment_variable_list, seps, assignments_one > {};
   struct method_name : seq< name > {};
   struct function_name : seq< list< name, one< '.' >, sep >, seps, opt_must< one< ':' >, seps, method_name, seps > > {};
   struct function_definition : if_must< key_function, seps, function_name, function_body > {};

   struct local_function : if_must< key_function, seps, name, seps, function_body > {};
   struct local_variables : if_must< name_list_must, seps, opt< assignments_one > > {};
   struct local_statement : if_must< key_local, seps, sor< local_function, local_variables > > {};

   struct semicolon : one< ';' > {};
   struct statement : sor< semicolon,
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
                                  function_definition,
                                  local_statement > {};

   struct grammar : must< opt< shebang >, statement_list< eof > > {};
// clang-format on

} // namespace lua53

#endif
#endif
