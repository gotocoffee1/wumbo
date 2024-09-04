#pragma once

#include <array>
#include <cassert>
#include <vector>

#include "ast.hpp"
#include "util.hpp"
#include "wasm_util.hpp"

namespace wumbo
{
using namespace ast;
using namespace wasm;

enum class var_type
{
    local,
    upvalue,
    global,
};

static value_types get_return_type(const expression& p)
{
    return std::visit(overload{
                          [](const nil&)
                          {
                              return value_types::nil;
                          },
                          [](const boolean& p)
                          {
                              return value_types::boolean;
                          },
                          [](const int_type& p)
                          {
                              return value_types::integer;
                          },
                          [](const float_type& p)
                          {
                              return value_types::number;
                          },
                          [](const literal& p)
                          {
                              return value_types::string;
                          },
                          [](const ellipsis& p)
                          {
                              return value_types::dynamic;
                          },
                          [](const function_body& p)
                          {
                              return value_types::function;
                          },
                          [](const table_constructor& p)
                          {
                              return value_types::table;
                          },
                          [](const box<bin_operation>& p)
                          {
                              auto lhs = get_return_type(p->lhs);
                              auto rhs = get_return_type(p->rhs);
                              if (lhs == value_types::integer && rhs == value_types::integer)
                              {
                                  switch (p->op)
                                  {
                                  case bin_operator::addition:
                                      break;
                                  case bin_operator::subtraction:
                                      break;
                                  case bin_operator::multiplication:
                                      break;
                                  case bin_operator::division:
                                      break;
                                  default:
                                      break;
                                  }
                                  return value_types::integer;
                              }

                              if (lhs == value_types::number && rhs == value_types::number)
                              {
                                  switch (p->op)
                                  {
                                  case bin_operator::addition:
                                      break;
                                  case bin_operator::subtraction:
                                      break;
                                  case bin_operator::multiplication:
                                      break;
                                  case bin_operator::division:
                                      break;
                                  default:
                                      break;
                                  }

                                  return value_types::number;
                              }
                              else
                              {
                                  if (lhs == value_types::integer && rhs == value_types::number)
                                  {
                                      return value_types::number;
                                  }
                                  if (lhs == value_types::number && rhs == value_types::integer)
                                  {
                                      return value_types::number;
                                  }
                              }

                              return value_types::dynamic;
                          },
                          [](const box<un_operation>& p)
                          {
                              auto rhs = get_return_type(p->rhs);

                              switch (p->op)
                              {
                              case un_operator::minus:
                                  if (rhs == value_types::number)
                                      return value_types::number;
                                  else if (rhs == value_types::integer)
                                      return value_types::integer;
                                  break;
                              case un_operator::logic_not:
                                  break;
                              case un_operator::len:
                                  return value_types::integer;
                              case un_operator::binary_not:
                                  break;
                              default:
                                  break;
                              }
                              return value_types::dynamic;
                          },

                          [](const auto&)
                          {
                              return value_types::dynamic;
                          },
                      } // namespace lua2wasm
                      ,
                      p.inner);
}

struct count_locals
{
    template<typename T>
    size_t operator()(const T&)
    {
        return 0;
    }

    size_t operator()(const do_statement& p)
    {
        return (*this)(p.inner);
    }
    size_t operator()(const while_statement& p)
    {
        return (*this)(p.inner);
    }
    size_t operator()(const repeat_statement& p)
    {
        return (*this)(p.inner);
    }
    size_t operator()(const if_statement& p)
    {
        size_t max_sub = p.else_block ? (*this)(*p.else_block) : 0;
        for (auto& [cond, block] : p.cond_block)
            max_sub = std::max((*this)(block), max_sub);

        return max_sub;
    }
    size_t operator()(const for_statement& p)
    {
        return (*this)(p.inner);
    }
    size_t operator()(const for_each& p)
    {
        return (*this)(p.inner);
    }

    size_t operator()(const block& p)
    {
        size_t locals = 0;
        for (auto& statement : p.statements)
        {
            locals += std::visit(overload{
                                     [](const local_function&) -> size_t
                                     {
                                         return 1;
                                     },

                                     [](const local_variables& p) -> size_t
                                     {
                                         return p.names.size();
                                     },
                                     [](const auto&) -> size_t
                                     {
                                         return 0;
                                     },
                                 },
                                 statement.inner);
        }

        size_t max_sub = 0;
        for (auto& statement : p.statements)
        {
            max_sub = std::max(std::visit(*this, statement.inner), max_sub);
        }

        return locals + max_sub;
    } // namespace lua2wasm
};

struct compiler : ext_types
{
    std::vector<std::string> vars;
    std::vector<size_t> blocks;
    std::vector<size_t> functions;

    std::vector<std::vector<size_t>> upvalues;

    std::vector<std::tuple<std::vector<BinaryenType>, std::vector<bool>, size_t>> util_locals;

    size_t function_name = 0;
    size_t data_name     = 0;
    size_t label_name    = 0;

    size_t local_offset(size_t index) const
    {
        return func_arg_count + (index - functions.back());
    }

    size_t local_offset() const
    {
        return local_offset(vars.size());
    }

    bool is_index_local(size_t index) const
    {
        return index >= functions.back();
    }

    void free_local(size_t pos)
    {
        auto& [types, used, offset] = util_locals.back();

        used[pos - (func_arg_count + offset)] = false;
    }

    size_t alloc_local(BinaryenType type)
    {
        auto& [types, used, offset] = util_locals.back();

        for (size_t i = 0; i < used.size(); ++i)
        {
            if (type == types[i] && !used[i])
            {
                used[i] = true;
                return func_arg_count + offset + i;
            }
        }
        auto size = types.size();
        types.push_back(type);
        used.push_back(true);

        return func_arg_count + offset + size;
    }

    std::tuple<var_type, size_t> find(const std::string& var_name) const
    {
        if (auto local = std::find(vars.rbegin(), vars.rend(), var_name); local != vars.rend())
        {
            auto pos = std::distance(vars.begin(), std::next(local).base());
            if (is_index_local(pos)) // local
                return {var_type::local, local_offset(pos)};
            else // upvalue
                return {var_type::upvalue, pos};
        }
        return {var_type::global, 0}; // global
    }

    BinaryenExpressionRef get_upvalue(size_t index)
    {
        auto exp = local_get(upvalue_index, upvalue_array_type());
        exp      = BinaryenArrayGet(mod, exp, const_i32(upvalues.back().size()), upvalue_type(), false);
        upvalues.back().push_back(index);
        return exp;
    }

    BinaryenExpressionRef get_var(const name_t& name)
    {
        auto [var_type, index] = find(name);
        switch (var_type)
        {
        case var_type::local:
            return BinaryenStructGet(mod, 0, local_get(index, upvalue_type()), anyref(), false);
        case var_type::upvalue:
            return BinaryenStructGet(mod, 0, get_upvalue(index), anyref(), false);
        case var_type::global:
            assert(name != "_ENV" && "no environment set");
            return table_get(get_var("_ENV"), add_string(name));
        default:
            return BinaryenUnreachable(mod);
        }
    }

    BinaryenExpressionRef set_var(const name_t& name, BinaryenExpressionRef value)
    {
        auto [var_type, index] = find(name);
        switch (var_type)
        {
        case var_type::local:
            return BinaryenStructSet(mod, 0, local_get(index, upvalue_type()), value);
        case var_type::upvalue:
            return BinaryenStructSet(mod, 0, get_upvalue(index), value);
        case var_type::global:
            return table_set(get_var("_ENV"), add_string(name), value);
        default:
            return BinaryenUnreachable(mod);
        }
    }

    BinaryenExpressionRef _funchead(const funchead& p);
    BinaryenExpressionRef _functail(const functail& p, BinaryenExpressionRef function);

    BinaryenExpressionRef _vartail(const vartail& p, BinaryenExpressionRef var);
    BinaryenExpressionRef _vartail_set(const vartail& p, BinaryenExpressionRef var, BinaryenExpressionRef value);

    BinaryenExpressionRef _varhead(const varhead& v);

    BinaryenExpressionRef _varhead_set(const varhead& v, BinaryenExpressionRef value);
    BinaryenExpressionRef at_or_null(size_t array, size_t index)
    {
        return BinaryenIf(mod,
                          BinaryenRefIsNull(mod, local_get(array, ref_array_type())),
                          null(),
                          BinaryenIf(mod,
                                     BinaryenBinary(mod, BinaryenGtUInt32(), BinaryenArrayLen(mod, local_get(array, ref_array_type())), const_i32(index)),
                                     BinaryenArrayGet(mod, local_get(array, ref_array_type()), const_i32(index), anyref(), false),
                                     null()));
    }

    std::vector<BinaryenExpressionRef> operator()(const function_call& p);

    std::vector<BinaryenExpressionRef> operator()(const assignments& p);

    auto operator()(const label_statement& p)
    {
        std::vector<BinaryenExpressionRef> result;
        return result;
    }
    auto operator()(const key_break& p)
    {
        std::vector<BinaryenExpressionRef> result;
        return result;
    }
    auto operator()(const goto_statement& p)
    {
        std::vector<BinaryenExpressionRef> result;
        return result;
    }
    auto operator()(const do_statement& p)
    {
        return std::vector<BinaryenExpressionRef>{(*this)(p.inner)};
    }
    auto operator()(const while_statement& p)
    {
        std::vector<BinaryenExpressionRef> result;
        return result;
    }
    auto operator()(const repeat_statement& p)
    {
        std::vector<BinaryenExpressionRef> result;
        return result;
    }
    auto operator()(const if_statement& p)
    {
        std::vector<BinaryenExpressionRef> result;
        return result;
    }
    auto operator()(const for_statement& p)
    {
        std::vector<BinaryenExpressionRef> result;
        return result;
    }
    auto operator()(const for_each& p)
    {
        std::vector<BinaryenExpressionRef> result;
        return result;
    }
    auto operator()(const function_definition& p) -> std::vector<BinaryenExpressionRef>
    {
        return {add_func_ref(p.function_name.back().c_str(), p.body)};
    }
    std::vector<BinaryenExpressionRef> operator()(const local_function& p);
    std::vector<BinaryenExpressionRef> operator()(const local_variables& p);

    BinaryenExpressionRef operator()(const expression& p);

    BinaryenExpressionRef operator()(const expression_list& p);

    auto operator()(const nil&)
    {
        return null();
    }

    auto operator()(const boolean& p)
    {
        return const_i32(p.value);
    }

    auto operator()(const int_type& p)
    {
        return BinaryenConst(mod, BinaryenLiteralInt64(p));
    }

    auto operator()(const float_type& p)
    {
        return BinaryenConst(mod, BinaryenLiteralFloat64(p));
    }

    BinaryenExpressionRef add_string(const std::string& str)
    {
        auto name = std::to_string(data_name++);
        BinaryenAddDataSegment(mod, name.c_str(), "", true, 0, str.data(), str.size());
        return BinaryenArrayNewData(mod, BinaryenTypeGetHeapType(type<value_types::string>()), name.c_str(), const_i32(0), const_i32(str.size()));
    }

    auto operator()(const literal& p)
    {
        return add_string(p.str);
    }

    auto operator()(const ellipsis& p)
    {
        return BinaryenUnreachable(mod);
    }

    std::vector<BinaryenExpressionRef> unpack_locals(const name_list& p, BinaryenExpressionRef list)
    {
        const auto offset              = local_offset();
        std::string none               = "+none" + std::to_string(label_name++);
        std::vector<const char*> names = {none.c_str()};
        bool is_upvalue                = true;
        size_t j                       = offset;
        std::vector<BinaryenExpressionRef> result;
        for (auto& arg : p)
        {
            vars.emplace_back(arg);
            names.push_back(arg.c_str());

            if (is_upvalue)
                result.push_back(local_set(j,
                                           BinaryenStructNew(
                                               mod,
                                               nullptr,
                                               0,
                                               BinaryenTypeGetHeapType(upvalue_type()))));
            j++;
        }
        std::array<BinaryenExpressionRef, 2> exp = {
            BinaryenSwitch(mod,
                           std::data(names),
                           std::size(names) - 1,
                           names.back(),
                           BinaryenArrayLen(mod,
                                            BinaryenBrOn(mod,
                                                         BinaryenBrOnNull(),
                                                         names[0],
                                                         list,
                                                         BinaryenTypeNone())),
                           nullptr),

        };
        j = p.size();

        for (auto iter = p.rbegin(); iter != p.rend(); ++iter)
        {
            j--;
            exp[0] = BinaryenBlock(mod, iter->c_str(), std::data(exp), iter == p.rbegin() ? 1 : std::size(exp), BinaryenTypeAuto());

            auto get = BinaryenArrayGet(mod,
                                        list,
                                        const_i32(j),
                                        anyref(),
                                        false);

            exp[1] = local_set(j + offset,
                               is_upvalue ? BinaryenStructNew(
                                   mod,
                                   &get,
                                   1,
                                   BinaryenTypeGetHeapType(upvalue_type()))
                                          : get);
        }

        result.push_back(make_block(exp, names[0]));
        return result;
    }

    auto add_func(const char* name, const block& inner, const name_list& p, bool vararg) -> BinaryenFunctionRef
    {
        functions.push_back(vars.size());

        //for (auto& name : p)
        //    scope.emplace_back(name);
        //vars.emplace_back("+upvalues");
        //vars.emplace_back("+args");

        std::vector<BinaryenType> locals(count_locals{}(inner) + p.size(), upvalue_type());
        std::get<2>(util_locals.emplace_back()) = locals.size();

        std::vector<BinaryenExpressionRef> body;
        if (!p.empty())
            body = unpack_locals(p, local_get(args_index, ref_array_type()));
        auto body_temp = (*this)(inner);

        body.insert(body.end(), body_temp.begin(), body_temp.end());

        body.push_back(BinaryenReturn(mod, null()));

        auto& types = std::get<0>(util_locals.back());
        locals.insert(locals.end(), types.begin(), types.end());

        auto result = BinaryenAddFunctionWithHeapType(mod,
                                                      name,
                                                      BinaryenTypeGetHeapType(lua_func()),
                                                      locals.data(),
                                                      locals.size(),
                                                      BinaryenBlock(mod, nullptr, body.data(), body.size(), BinaryenTypeAuto()));

        BinaryenFunctionSetLocalName(result, args_index, "args");
        BinaryenFunctionSetLocalName(result, upvalue_index, "upvalues");

        size_t i = func_arg_count;
        for (auto& arg : p)
        {
            BinaryenFunctionSetLocalName(result, i++, arg.c_str());
        }

        //for (size_t i = functions.back() + 2; i < vars.size(); ++i)
        //    BinaryenFunctionSetLocalName(result, i - functions.back(), vars[i].c_str());

        vars.resize(functions.back());
        functions.pop_back();
        util_locals.pop_back();

        return result;
    }

    auto add_func_ref(const char* name, const function_body& p) -> BinaryenExpressionRef
    {
        upvalues.emplace_back();
        auto func = add_func(name, p.inner, p.params, p.vararg);

        auto sig = BinaryenTypeFromHeapType(BinaryenFunctionGetType(func), false);

        std::vector<BinaryenExpressionRef> ups;

        auto req_ups = std::move(upvalues.back());
        upvalues.pop_back();
        for (auto index : req_ups)
        {
            if (is_index_local(index)) // local
            {
                ups.push_back(local_get(local_offset(index), upvalue_type()));
            }
            else // upvalue
            {
                ups.push_back(get_upvalue(index));
            }
        }
        BinaryenExpressionRef exp[] = {
            BinaryenRefFunc(mod, name, sig),
            ups.empty() ? null() : BinaryenArrayNewFixed(mod, BinaryenTypeGetHeapType(upvalue_array_type()), std::data(ups), std::size(ups)),
        };
        return BinaryenStructNew(mod, std::data(exp), std::size(exp), BinaryenTypeGetHeapType(type<value_types::function>()));
    }

    auto operator()(const function_body& p)
    {
        return add_func_ref(std::to_string(function_name++).c_str(), p);
    }

    BinaryenExpressionRef operator()(const prefixexp& p);

    template<typename T>
    auto operator()(const box<T>& p)
    {
        return (*this)(*p);
    }

    BinaryenExpressionRef calc_hash(BinaryenExpressionRef key);
    BinaryenExpressionRef find_bucket(BinaryenExpressionRef table, BinaryenExpressionRef hash);

    BinaryenExpressionRef table_get(BinaryenExpressionRef table, BinaryenExpressionRef key);

    BinaryenExpressionRef table_set(BinaryenExpressionRef table, BinaryenExpressionRef key, BinaryenExpressionRef value);

    BinaryenFunctionRef compare(const char* name, value_types vtype);
    void func_table_get();
    BinaryenExpressionRef operator()(const table_constructor& p);

    auto operator()(const bin_operation& p)
    {
        auto lhs_type = get_return_type(p.lhs);
        auto rhs_type = get_return_type(p.rhs);
        BinaryenExpressionRef result;

        auto lhs = (*this)(p.lhs);
        auto rhs = (*this)(p.rhs);

        BinaryenOp op;

        if (lhs_type == value_types::integer && rhs_type == value_types::integer)
        {
            switch (p.op)
            {
            case bin_operator::addition:
                op = BinaryenAddInt64();
                break;
            case bin_operator::subtraction:
                op = BinaryenSubInt64();
                break;
            case bin_operator::multiplication:
                op = BinaryenMulInt64();
                break;
            case bin_operator::division:
                op = BinaryenDivSInt64();
                break;
            default:
                break;
            }
        }
        else if (lhs_type == value_types::number || rhs_type == value_types::number)
        {
            switch (p.op)
            {
            case bin_operator::addition:
                op = BinaryenAddFloat64();
                break;
            case bin_operator::subtraction:
                op = BinaryenSubFloat64();
                break;
            case bin_operator::multiplication:
                op = BinaryenMulFloat64();
                break;
            case bin_operator::division:
                op = BinaryenDivFloat64();
                break;
            default:
                break;
            }

            if (lhs_type == value_types::integer)
            {
                lhs = BinaryenUnary(mod, BinaryenConvertSInt64ToFloat64(), lhs);
            }
            else if (lhs_type == value_types::number)
            {
                rhs = BinaryenUnary(mod, BinaryenConvertSInt64ToFloat64(), rhs);
            }
        }

        result = BinaryenBinary(mod, op, lhs, rhs);
        return result;
    }

    auto operator()(const un_operation& p)
    {
        auto rhs_type = get_return_type(p.rhs);

        auto rhs = (*this)(p.rhs);
        return rhs;
    }

    auto operator()(const block& p) -> std::vector<BinaryenExpressionRef>
    {
        blocks.push_back(vars.size());

        std::vector<BinaryenExpressionRef> result;
        for (auto& statement : p.statements)
        {
            auto temp = std::visit(*this, statement.inner);
            result.insert(result.end(), temp.begin(), temp.end());
        }
        if (p.retstat)
        {
            auto list = (*this)(*p.retstat);
            result.push_back(BinaryenReturn(mod, list));
        }

        vars.resize(blocks.back());
        blocks.pop_back();
        return result;
    }

    auto convert()
    {
        func_table_get();
        BinaryenAddFunctionImport(mod, "print_integer", "print", "value", integer_type(), BinaryenTypeNone());
        BinaryenAddFunctionImport(mod, "print_number", "print", "value", number_type(), BinaryenTypeNone());
        BinaryenAddFunctionImport(mod, "print_nil", "print", "value", BinaryenTypeNullref(), BinaryenTypeNone());

        BinaryenExpressionRef args[] = {null(), null()};
        auto exp                     = BinaryenCall(mod, "*init", std::data(args), std::size(args), ref_array_type());
        exp                          = BinaryenArrayGet(mod, exp, const_i32(0), anyref(), false);

        std::vector<std::tuple<const char*, value_types>> casts = {
            {
                "number",
                value_types::number,
            },
            {
                "integer",
                value_types::integer,
            },
        };

        auto s = [&](value_types type, BinaryenExpressionRef exp)
        {
            const char* func;
            switch (type)
            {
            case value_types::nil:
                exp  = null();
                func = "print_nil";
                break;
            case value_types::boolean:
                exp  = null();
                func = "print_nil";
                break;
            case value_types::integer:
                func = "print_integer";
                exp  = BinaryenStructGet(mod, 0, exp, integer_type(), false);
                break;
            case value_types::number:
                func = "print_number";
                exp  = BinaryenStructGet(mod, 0, exp, number_type(), false);
                break;
            case value_types::string:
                func = "print_string";
                exp  = BinaryenStructGet(mod, 0, exp, number_type(), false);
                break;
            case value_types::function:
            case value_types::userdata:
            case value_types::thread:
            case value_types::table:
            case value_types::dynamic:
            default:
                return BinaryenUnreachable(mod);
            }
            return BinaryenReturnCall(mod, func, &exp, 1, BinaryenTypeNone());
        };

        BinaryenType locals[] = {anyref()};
        return BinaryenAddFunction(mod,
                                   "convert",
                                   BinaryenTypeNone(),
                                   BinaryenTypeNone(),
                                   std::data(locals),
                                   std::size(locals),
                                   make_block(switch_value(exp, casts, s)));
    }
};
} // namespace wumbo