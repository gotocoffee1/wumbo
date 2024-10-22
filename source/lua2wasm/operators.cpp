#include "compiler.hpp"

namespace wumbo
{

void compiler::make_bin_operation()
{
    auto make_func = [&](const char* function, const auto& casts, auto&& op)
    {
        BinaryenType p[] = {anyref(), anyref()};
        auto params      = BinaryenTypeCreate(std::data(p), std::size(p));
        auto vars        = std::array<BinaryenType, 0>{};

        BinaryenAddFunction(mod,
                            function,
                            params,
                            anyref(),
                            std::data(vars),
                            std::size(vars),
                            make_block(switch_value(local_get(0, anyref()), casts, [&](value_types left_type, expr_ref left)
                                                    {
                                                        if (std::find(std::begin(casts), std::end(casts), left_type) == std::end(casts))
                                                            return throw_error(add_string("unexpected type"));

                                                        auto func_name = std::string{function} + to_string(left_type);

                                                        auto p = std::array{type(left_type), anyref()};

                                                        auto params = BinaryenTypeCreate(std::data(p), std::size(p));

                                                        BinaryenAddFunction(mod,
                                                                            func_name.c_str(),
                                                                            params,
                                                                            anyref(),
                                                                            std::data(vars),
                                                                            std::size(vars),
                                                                            make_block(switch_value(local_get(1, anyref()), casts, [&](value_types right_type, expr_ref right)
                                                                                                    {
                                                                                                        auto left = local_get(0, type(left_type));

                                                                                                        switch (left_type)
                                                                                                        {
                                                                                                        case value_types::integer:
                                                                                                            left = BinaryenStructGet(mod, 0, left, integer_type(), false);
                                                                                                            switch (right_type)
                                                                                                            {
                                                                                                            case value_types::integer:
                                                                                                                right = BinaryenStructGet(mod, 0, right, integer_type(), false);
                                                                                                                break;
                                                                                                            case value_types::number:
                                                                                                                right = BinaryenStructGet(mod, 0, right, number_type(), false);
                                                                                                                break;
                                                                                                            default:
                                                                                                                return throw_error(add_string("unexpected type"));
                                                                                                            }
                                                                                                            break;

                                                                                                        case value_types::number:
                                                                                                            left = BinaryenStructGet(mod, 0, left, number_type(), false);
                                                                                                            switch (right_type)
                                                                                                            {
                                                                                                            case value_types::integer:
                                                                                                                right = BinaryenStructGet(mod, 0, right, integer_type(), false);
                                                                                                                break;
                                                                                                            case value_types::number:
                                                                                                                right = BinaryenStructGet(mod, 0, right, number_type(), false);
                                                                                                                break;
                                                                                                            default:
                                                                                                                return throw_error(add_string("unexpected type"));
                                                                                                            }
                                                                                                            break;

                                                                                                        default:
                                                                                                            semantic_error("unreachable");
                                                                                                        }

                                                                                                        return op(left_type, right_type, left, right);
                                                                                                    })));

                                                        auto args = std::array{left, local_get(1, anyref())};
                                                        return BinaryenReturnCall(mod, func_name.c_str(), std::data(args), std::size(args), anyref());
                                                    })));
    };

    {
        auto casts = std::array{
            value_types::integer,
            value_types::number,
        };

        auto functions = std::array{
            std::tuple{"*addition", &compiler::add_int, &compiler::add_num},
            std::tuple{"*subtraction", &compiler::sub_int, &compiler::sub_num},
            std::tuple{"*multiplication", &compiler::mul_int, &compiler::mul_num},
            std::tuple{"*division", &compiler::div_int, &compiler::div_num},
            std::tuple{"*division_floor", &compiler::div_int, &compiler::div_num},
            std::tuple{"*exponentiation", &compiler::div_int, &compiler::div_num},
            std::tuple{"*modulo", &compiler::rem_int, &compiler::div_num},
        };

        for (auto& [function, int_op, num_op] : functions)
        {
            make_func(function, casts, [&](value_types left_type, value_types right_type, expr_ref left, expr_ref right)
                      {
                          switch (left_type)
                          {
                          case value_types::integer:
                              switch (right_type)
                              {
                              case value_types::integer:
                                  return make_return(new_integer(std::invoke(int_op, this, left, right)));
                              case value_types::number:
                                  left = int_to_num(left);
                                  return make_return(new_number(std::invoke(num_op, this, left, right)));
                              default:
                                  return throw_error(add_string("unexpected type"));
                              }

                          case value_types::number:
                              switch (right_type)
                              {
                              case value_types::integer:
                                  right = int_to_num(right);
                                  return make_return(new_number(std::invoke(num_op, this, left, right)));
                              case value_types::number:
                                  return make_return(new_number(std::invoke(num_op, this, left, right)));
                              default:
                                  return throw_error(add_string("unexpected type"));
                              }
                          default:
                              semantic_error("unreachable");
                          }
                      });
        }

        auto bitop = std::array{
            std::tuple{"*binary_or", &compiler::or_int},
            std::tuple{"*binary_and", &compiler::and_int},
            std::tuple{"*binary_xor", &compiler::xor_int},
            std::tuple{"*binary_right_shift", &compiler::shr_int},
            std::tuple{"*binary_left_shift", &compiler::shl_int},
        };

        for (auto& [function, int_op] : bitop)
        {
            make_func(function, casts, [&](value_types left_type, value_types right_type, expr_ref left, expr_ref right)
                      {
                          switch (left_type)
                          {
                          case value_types::integer:
                              switch (right_type)
                              {
                              case value_types::integer:
                                  return make_return(new_integer(std::invoke(int_op, this, left, right)));
                              case value_types::number:
                                  //left = int_to_num(left);
                                  //return make_return(new_number(std::invoke(int_op, this, left, right)));
                              default:
                                  return throw_error(add_string("unexpected type"));
                              }

                          case value_types::number:
                              switch (right_type)
                              {
                              case value_types::integer:
                                  //right = int_to_num(right);
                                  //return make_return(new_number(std::invoke(int_op, this, left, right)));
                              case value_types::number:
                                  //return make_return(new_number(std::invoke(int_op, this, left, right)));
                              default:
                                  return throw_error(add_string("unexpected type"));
                              }
                          default:
                              semantic_error("unreachable");
                          }
                      });
        }

        auto relational = std::array{
            std::tuple{"*equality", &compiler::eq_int, &compiler::eq_num},
            std::tuple{"*inequality", &compiler::ne_int, &compiler::ne_num},
            std::tuple{"*less_than", &compiler::lt_int, &compiler::lt_num},
            std::tuple{"*greater_than", &compiler::gt_int, &compiler::ge_num},
            std::tuple{"*less_or_equal", &compiler::le_int, &compiler::le_num},
            std::tuple{"*greater_or_equal", &compiler::ge_int, &compiler::ge_num},
        };

        for (auto& [function, int_op, num_op] : relational)
        {
            make_func(function, casts, [&](value_types left_type, value_types right_type, expr_ref left, expr_ref right)
                      {
                          switch (left_type)
                          {
                          case value_types::integer:
                              switch (right_type)
                              {
                              case value_types::integer:
                                  return make_return(new_boolean(std::invoke(int_op, this, left, right)));
                              case value_types::number:
                                  left = int_to_num(left);
                                  return make_return(new_boolean(std::invoke(num_op, this, left, right)));
                              default:
                                  return throw_error(add_string("unexpected type"));
                              }

                          case value_types::number:
                              switch (right_type)
                              {
                              case value_types::integer:
                                  right = int_to_num(right);
                                  return make_return(new_boolean(std::invoke(num_op, this, left, right)));
                              case value_types::number:
                                  return make_return(new_boolean(std::invoke(num_op, this, left, right)));
                              default:
                                  return throw_error(add_string("unexpected type"));
                              }
                          default:
                              semantic_error("unreachable");
                          }
                      });
        }
    }
}

// TODO
expr_ref compiler::operator()(const bin_operation& p)
{
    auto lhs = (*this)(p.lhs);
    auto rhs = (*this)(p.rhs);

    if (BinaryenExpressionGetType(lhs) == ref_array_type())
    {
        auto local = help_var_scope{_func_stack, ref_array_type()};
        lhs        = at_or_null(local, 0, lhs);
    }
    if (BinaryenExpressionGetType(rhs) == ref_array_type())
    {
        auto local = help_var_scope{_func_stack, ref_array_type()};
        rhs        = at_or_null(local, 0, rhs);
    }
    switch (p.op)
    {
    case bin_operator::logic_and:
    {
        auto left = help_var_scope{_func_stack, anyref()};
        return make_if(make_call("*to_bool", local_tee(left, lhs, anyref()), bool_type()), rhs, local_get(left, anyref()));
    }

    case bin_operator::logic_or:
    {
        auto left = help_var_scope{_func_stack, anyref()};
        return make_if(make_call("*to_bool", local_tee(left, lhs, anyref()), bool_type()), local_get(left, anyref()), rhs);
    }
    };

    const char* func = [&]()
    {
        switch (p.op)
        {
        case bin_operator::addition:
            return "*addition";
        case bin_operator::subtraction:
            return "*subtraction";
        case bin_operator::multiplication:
            return "*multiplication";
        case bin_operator::division:
            return "*division";
        case bin_operator::division_floor:
            return "*division_floor";
        case bin_operator::exponentiation:
            return "*exponentiation";
        case bin_operator::modulo:
            return "*modulo";

        case bin_operator::binary_or:
            return "*binary_or";
        case bin_operator::binary_and:
            return "*binary_and";
        case bin_operator::binary_xor:
            return "*binary_xor";
        case bin_operator::binary_right_shift:
            return "*binary_right_shift";
        case bin_operator::binary_left_shift:
            return "*binary_left_shift";

        case bin_operator::equality:
            return "*equality";
        case bin_operator::inequality:
            return "*inequality";
        case bin_operator::less_than:
            return "*less_than";
        case bin_operator::greater_than:
            return "*greater_than";
        case bin_operator::less_or_equal:
            return "*less_or_equal";
        case bin_operator::greater_or_equal:
            return "*greater_or_equal";

        case bin_operator::concat:
            return "*concat";
        default:
            semantic_error("");
        }
    }();

    return make_call(func, std::array{lhs, rhs}, anyref());
}

void compiler::make_un_operation()
{
    auto vars = std::array<BinaryenType, 0>{};

    {
        BinaryenAddFunction(mod,
                            "*logic_not",
                            anyref(),
                            anyref(),
                            std::data(vars),
                            std::size(vars),
                            BinaryenRefI31(mod, make_call("*to_bool_invert", local_get(0, anyref()), bool_type())));
    }
    {
        auto casts = std::array{
            value_types::integer,
            value_types::number,
        };

        BinaryenAddFunction(mod,
                            "*binary_not",
                            anyref(),
                            anyref(),
                            std::data(vars),
                            std::size(vars),
                            make_block(switch_value(local_get(0, anyref()), casts, [&](value_types type, expr_ref exp)
                                                    {
                                                        switch (type)
                                                        {
                                                        case value_types::integer:
                                                            exp = BinaryenStructGet(mod, 0, exp, integer_type(), false);
                                                            return make_return(new_integer(xor_int(const_integer(-1), exp)));
                                                        case value_types::number:
                                                            // TODO
                                                            return make_return(exp);
                                                        default:
                                                            return throw_error(add_string("unexpected type"));
                                                        }
                                                    })));
    }
    {
        auto casts = std::array{
            value_types::integer,
            value_types::number,
        };
        BinaryenAddFunction(mod,
                            "*minus",
                            anyref(),
                            anyref(),
                            std::data(vars),
                            std::size(vars),
                            make_block(switch_value(local_get(0, anyref()), casts, [&](value_types type, expr_ref exp)
                                                    {
                                                        switch (type)
                                                        {
                                                        case value_types::integer:
                                                            exp = BinaryenStructGet(mod, 0, exp, integer_type(), false);
                                                            return make_return(new_integer(mul_int(const_integer(-1), exp)));
                                                        case value_types::number:
                                                            exp = BinaryenStructGet(mod, 0, exp, number_type(), false);
                                                            return make_return(new_number(neg_num(exp)));
                                                        default:
                                                            return throw_error(add_string("unexpected type"));
                                                        }
                                                    })));
    }

    {
        auto casts = std::array{
            value_types::string,
            value_types::table,
            //value_types::userdata,
        };

        BinaryenAddFunction(mod,
                            "*len",
                            anyref(),
                            anyref(),
                            std::data(vars),
                            std::size(vars),
                            make_block(switch_value(local_get(0, anyref()), casts, [&](value_types type, expr_ref exp)
                                                    {
                                                        switch (type)
                                                        {
                                                        case value_types::string:
                                                            return make_return(new_integer(size_to_integer(array_len(exp))));
                                                        case value_types::table:
                                                        case value_types::userdata:
                                                        {
                                                            // TODO

                                                            return drop(exp);
                                                        }
                                                        default:

                                                            return throw_error(add_string("unexpected type"));
                                                        }
                                                    })));
    }
}

expr_ref compiler::operator()(const un_operation& p)
{
    auto rhs = (*this)(p.rhs);

    if (BinaryenExpressionGetType(rhs) == ref_array_type())
    {
        auto local = help_var_scope{_func_stack, ref_array_type()};
        rhs        = at_or_null(local, 0, rhs);
    }
    switch (p.op)
    {
    case un_operator::minus:
        return make_call("*minus", rhs, anyref());
    case un_operator::logic_not:
        return make_call("*logic_not", rhs, anyref());
    case un_operator::len:
        return make_call("*len", rhs, anyref());
    case un_operator::binary_not:
        return make_call("*binary_not", rhs, anyref());
    default:
        semantic_error("");
        break;
    }
}
} // namespace wumbo