#include "ast/ast.hpp"
#include "compiler.hpp"

#include <functional>

namespace wumbo
{

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
        return make_if(_runtime.call(functions::to_bool, local_tee(left, lhs, anyref())), rhs, local_get(left, anyref()));
    }

    case bin_operator::logic_or:
    {
        auto left = help_var_scope{_func_stack, anyref()};
        return make_if(_runtime.call(functions::to_bool, local_tee(left, lhs, anyref())), local_get(left, anyref()), rhs);
    }
    default:
        break;
    };

    auto func = [this](bin_operator op)
    {
        switch (op)
        {
        case bin_operator::addition:
            return functions::addition;
        case bin_operator::subtraction:
            return functions::subtraction;
        case bin_operator::multiplication:
            return functions::multiplication;
        case bin_operator::division:
            return functions::division;
        case bin_operator::division_floor:
            return functions::division_floor;
        case bin_operator::exponentiation:
            return functions::exponentiation;
        case bin_operator::modulo:
            return functions::modulo;

        case bin_operator::binary_or:
            return functions::binary_or;
        case bin_operator::binary_and:
            return functions::binary_and;
        case bin_operator::binary_xor:
            return functions::binary_xor;
        case bin_operator::binary_right_shift:
            return functions::binary_right_shift;
        case bin_operator::binary_left_shift:
            return functions::binary_left_shift;

        case bin_operator::equality:
            return functions::equality;
        case bin_operator::inequality:
            return functions::inequality;
        case bin_operator::less_than:
            return functions::less_than;
        case bin_operator::greater_than:
            return functions::greater_than;
        case bin_operator::less_or_equal:
            return functions::less_or_equal;
        case bin_operator::greater_or_equal:
            return functions::greater_or_equal;

        //case bin_operator::concat:
        //    return functions::concat;
        default:
            semantic_error("");
        }
    }(p.op);

    return _runtime.call(func, std::array{lhs, rhs});
}

expr_ref compiler::operator()(const un_operation& p)
{
    auto rhs = (*this)(p.rhs);

    if (BinaryenExpressionGetType(rhs) == ref_array_type())
    {
        auto local = help_var_scope{_func_stack, ref_array_type()};
        rhs        = at_or_null(local, 0, rhs);
    }
    functions f = [this](un_operator op)
    {
        switch (op)
        {
        case un_operator::minus:
            return functions::minus;
        case un_operator::logic_not:
            return functions::logic_not;
        case un_operator::len:
            return functions::len;
        case un_operator::binary_not:
            return functions::binary_not;
        default:
            semantic_error("");
            break;
        }
    }(p.op);

    return _runtime.call(f, rhs);
}
} // namespace wumbo
