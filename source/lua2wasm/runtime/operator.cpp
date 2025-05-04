#include "binaryen-c.h"
#include "runtime.hpp"
#include "wasm_util.hpp"

#include <functional>

namespace wumbo
{

struct runtime::op
{
    struct expo
    {
        static constexpr auto casts = std::array{
            value_type::integer,
            value_type::number,
        };

        expr_ref operator()(runtime* self, value_type left_type, value_type right_type, expr_ref left, expr_ref right)
        {
            switch (left_type)
            {
            case value_type::integer:
                left = self->int_to_num(left);
                switch (right_type)
                {
                case value_type::integer:
                    right = self->int_to_num(right);
                case value_type::number:
                    return self->make_return(self->new_number(self->make_call("pow", std::array{left, right}, number_type())));
                default:
                    return self->throw_error(self->add_string("unexpected type"));
                }

            case value_type::number:
                switch (right_type)
                {
                case value_type::integer:
                    right = self->int_to_num(right);
                case value_type::number:
                    return self->make_return(self->new_number(self->make_call("pow", std::array{left, right}, number_type())));
                default:
                    return self->throw_error(self->add_string("unexpected type"));
                }
            default:
                self->semantic_error("unreachable");
            }
        }
    };

    struct arith
    {
        expr_ref (runtime::*int_op)(expr_ref, expr_ref);
        expr_ref (runtime::*num_op)(expr_ref, expr_ref);

        static constexpr auto casts = std::array{
            value_type::integer,
            value_type::number,
        };

        expr_ref operator()(runtime* self, value_type left_type, value_type right_type, expr_ref left, expr_ref right)
        {
            switch (left_type)
            {
            case value_type::integer:
                switch (right_type)
                {
                case value_type::integer:
                    return self->make_return(self->new_integer(std::invoke(int_op, self, left, right)));
                case value_type::number:
                    left = self->int_to_num(left);
                    return self->make_return(self->new_number(std::invoke(num_op, self, left, right)));
                default:
                    return self->throw_error(self->add_string("unexpected type"));
                }

            case value_type::number:
                switch (right_type)
                {
                case value_type::integer:
                    right = self->int_to_num(right);
                    return self->make_return(self->new_number(std::invoke(num_op, self, left, right)));
                case value_type::number:
                    return self->make_return(self->new_number(std::invoke(num_op, self, left, right)));
                default:
                    return self->throw_error(self->add_string("unexpected type"));
                }
            default:
                self->semantic_error("unreachable");
            }
        }
    };

    struct cmp
    {
        expr_ref (runtime::*int_op)(expr_ref, expr_ref);
        expr_ref (runtime::*num_op)(expr_ref, expr_ref);

        static constexpr auto casts = std::array{
            value_type::integer,
            value_type::number,
        };

        expr_ref operator()(runtime* self, value_type left_type, value_type right_type, expr_ref left, expr_ref right)
        {
            switch (left_type)
            {
            case value_type::integer:
                switch (right_type)
                {
                case value_type::integer:
                    return self->make_return(self->new_boolean(std::invoke(int_op, self, left, right)));
                case value_type::number:
                    left = self->int_to_num(left);
                    return self->make_return(self->new_boolean(std::invoke(num_op, self, left, right)));
                default:
                    return self->throw_error(self->add_string("unexpected type"));
                }

            case value_type::number:
                switch (right_type)
                {
                case value_type::integer:
                    right = self->int_to_num(right);
                    return self->make_return(self->new_boolean(std::invoke(num_op, self, left, right)));
                case value_type::number:
                    return self->make_return(self->new_boolean(std::invoke(num_op, self, left, right)));
                default:
                    return self->throw_error(self->add_string("unexpected type"));
                }
            default:
                self->semantic_error("unreachable");
            }
        }
    };

    struct bit
    {
        expr_ref (runtime::*int_op)(expr_ref, expr_ref);

        static constexpr auto casts = std::array{
            value_type::integer,
            value_type::number,
        };

        expr_ref operator()(runtime* self, value_type left_type, value_type right_type, expr_ref left, expr_ref right)
        {
            switch (left_type)
            {
            case value_type::integer:
                switch (right_type)
                {
                case value_type::integer:
                    return self->make_return(self->new_integer(std::invoke(int_op, self, left, right)));
                case value_type::number:
                    //left = int_to_num(left);
                    //return make_return(new_number(std::invoke(int_op, this, left, right)));
                default:
                    return self->throw_error(self->add_string("unexpected type"));
                }

            case value_type::number:
                switch (right_type)
                {
                case value_type::integer:
                    //right = int_to_num(right);
                    //return make_return(new_number(std::invoke(int_op, this, left, right)));
                case value_type::number:
                    //return make_return(new_number(std::invoke(int_op, this, left, right)));
                default:
                    return self->throw_error(self->add_string("unexpected type"));
                }
            default:
                self->semantic_error("unreachable");
            }
        }
    };

    template<typename F>
    static build_return_t bin(runtime* self, const char* function, F&& op)
    {
        return {std::vector<BinaryenType>{},
                self->make_block(self->switch_value(self->local_get(0, anyref()), op.casts, [&](value_type left_type, expr_ref left)
                                                    {
                                                        if (std::find(std::begin(op.casts), std::end(op.casts), left_type) == std::end(op.casts))
                                                            return self->throw_error(self->add_string("unexpected type"));

                                                        auto func_name = std::string{function} + type_name(left_type);

                                                        auto p = std::array{self->type(left_type), anyref()};

                                                        auto params = BinaryenTypeCreate(std::data(p), std::size(p));

                                                        BinaryenAddFunction(self->mod,
                                                                            func_name.c_str(),
                                                                            params,
                                                                            anyref(),
                                                                            nullptr,
                                                                            0,
                                                                            self->make_block(self->switch_value(self->local_get(1, anyref()), op.casts, [&](value_type right_type, expr_ref right)
                                                                                                                {
                                                                                                                    auto left = self->local_get(0, self->type(left_type));

                                                                                                                    switch (left_type)
                                                                                                                    {
                                                                                                                    case value_type::integer:
                                                                                                                        left = BinaryenStructGet(self->mod, 0, left, self->integer_type(), false);
                                                                                                                        switch (right_type)
                                                                                                                        {
                                                                                                                        case value_type::integer:
                                                                                                                            right = BinaryenStructGet(self->mod, 0, right, self->integer_type(), false);
                                                                                                                            break;
                                                                                                                        case value_type::number:
                                                                                                                            right = BinaryenStructGet(self->mod, 0, right, self->number_type(), false);
                                                                                                                            break;
                                                                                                                        default:
                                                                                                                            return self->throw_error(self->add_string("unexpected type"));
                                                                                                                        }
                                                                                                                        break;

                                                                                                                    case value_type::number:
                                                                                                                        left = BinaryenStructGet(self->mod, 0, left, self->number_type(), false);
                                                                                                                        switch (right_type)
                                                                                                                        {
                                                                                                                        case value_type::integer:
                                                                                                                            right = BinaryenStructGet(self->mod, 0, right, self->integer_type(), false);
                                                                                                                            break;
                                                                                                                        case value_type::number:
                                                                                                                            right = BinaryenStructGet(self->mod, 0, right, self->number_type(), false);
                                                                                                                            break;
                                                                                                                        default:
                                                                                                                            return self->throw_error(self->add_string("unexpected type"));
                                                                                                                        }
                                                                                                                        break;

                                                                                                                    default:
                                                                                                                        self->semantic_error("unreachable");
                                                                                                                    }

                                                                                                                    return op(self, left_type, right_type, left, right);
                                                                                                                })));

                                                        auto args = std::array{left, self->local_get(1, anyref())};
                                                        return BinaryenReturnCall(self->mod, func_name.c_str(), std::data(args), std::size(args), anyref());
                                                    }))};
    }
};

build_return_t runtime::addition()
{
    return op::bin(this, "addition", op::arith{&runtime::add_int, &runtime::add_num});
}

build_return_t runtime::subtraction()
{
    return op::bin(this, "subtraction", op::arith{&runtime::sub_int, &runtime::sub_num});
}

build_return_t runtime::multiplication()
{
    return op::bin(this, "multiplication", op::arith{&runtime::mul_int, &runtime::mul_num});
}

build_return_t runtime::division()
{
    return op::bin(this, "division", op::arith{&runtime::div_int, &runtime::div_num});
}

build_return_t runtime::division_floor()
{
    return op::bin(this, "division_floor", op::arith{&runtime::div_int, &runtime::div_num});
}

build_return_t runtime::exponentiation()
{
    import_func("pow", create_type(number_type(), number_type()), number_type(), "native", "pow");
    return op::bin(this, "exponentiation", op::expo{});
}

build_return_t runtime::modulo()
{
    return op::bin(this, "modulo", op::arith{&runtime::rem_int, &runtime::div_num});
}

build_return_t runtime::binary_or()
{
    return op::bin(this, "binary_or", op::bit{&runtime::or_int});
}

build_return_t runtime::binary_and()
{
    return op::bin(this, "binary_and", op::bit{&runtime::and_int});
}

build_return_t runtime::binary_xor()
{
    return op::bin(this, "binary_xor", op::bit{&runtime::xor_int});
}

build_return_t runtime::binary_right_shift()
{
    return op::bin(this, "binary_right_shift", op::bit{&runtime::shr_int});
}

build_return_t runtime::binary_left_shift()
{
    return op::bin(this, "binary_left_shift", op::bit{&runtime::shl_int});
}

build_return_t runtime::equality()
{
    return op::bin(this, "equality", op::cmp{&runtime::eq_int, &runtime::eq_num});
}

build_return_t runtime::inequality()
{
    return op::bin(this, "inequality", op::cmp{&runtime::ne_int, &runtime::ne_num});
}

build_return_t runtime::less_than()
{
    return op::bin(this, "less_than", op::cmp{&runtime::lt_int, &runtime::lt_num});
}

build_return_t runtime::greater_than()
{
    return op::bin(this, "greater_than", op::cmp{&runtime::gt_int, &runtime::ge_num});
}

build_return_t runtime::less_or_equal()
{
    return op::bin(this, "less_or_equal", op::cmp{&runtime::le_int, &runtime::le_num});
}

build_return_t runtime::greater_or_equal()
{
    return op::bin(this, "greater_or_equal", op::cmp{&runtime::ge_int, &runtime::ge_num});
}

build_return_t runtime::logic_not()
{
    return {std::vector<BinaryenType>{},
            BinaryenRefI31(mod, call(functions::to_bool_not, local_get(0, anyref())))};
}

build_return_t runtime::binary_not()
{
    auto casts = std::array{
        value_type::integer,
        value_type::number,
    };

    return {std::vector<BinaryenType>{},
            make_block(switch_value(local_get(0, anyref()), casts, [&](value_type type, expr_ref exp)
                                    {
                                        switch (type)
                                        {
                                        case value_type::integer:
                                            exp = BinaryenStructGet(mod, 0, exp, integer_type(), false);
                                            return make_return(new_integer(xor_int(const_integer(-1), exp)));
                                        case value_type::number:
                                            // TODO
                                            return make_return(exp);
                                        default:
                                            return throw_error(add_string("unexpected type"));
                                        }
                                    }))};
}

build_return_t runtime::minus()
{
    auto casts = std::array{
        value_type::integer,
        value_type::number,
    };
    return {std::vector<BinaryenType>{},
            make_block(switch_value(local_get(0, anyref()), casts, [&](value_type type, expr_ref exp)
                                    {
                                        switch (type)
                                        {
                                        case value_type::integer:
                                            exp = BinaryenStructGet(mod, 0, exp, integer_type(), false);
                                            return make_return(new_integer(mul_int(const_integer(-1), exp)));
                                        case value_type::number:
                                            exp = BinaryenStructGet(mod, 0, exp, number_type(), false);
                                            return make_return(new_number(neg_num(exp)));
                                        default:
                                            return throw_error(add_string("unexpected type"));
                                        }
                                    }))};
}

build_return_t runtime::len()
{
    auto casts = std::array{
        value_type::string,
        value_type::table,
        //value_type::userdata,
    };

    return {std::vector<BinaryenType>{},
            make_block(switch_value(local_get(0, anyref()), casts, [&](value_type type, expr_ref exp)
                                    {
                                        switch (type)
                                        {
                                        case value_type::string:
                                            return make_return(new_integer(size_to_integer(array_len(exp))));
                                        case value_type::table:
                                        case value_type::userdata:
                                        {
                                            // TODO

                                            return drop(exp);
                                        }
                                        default:

                                            return throw_error(add_string("unexpected type"));
                                        }
                                    }))};
}
} // namespace wumbo
