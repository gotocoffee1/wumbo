#include "lua2wasm.hpp"

#include <stack>
#include <unordered_map>

#include "util.hpp"

#include "wasm_util.hpp"

namespace lua2wasm
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

struct max_returns
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
        size_t returns = p.else_block ? (*this)(*p.else_block) : 0;
        for (auto& [cond, block] : p.cond_block)
            returns = std::max((*this)(block), returns);

        return returns;
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
        size_t returns = p.retstat ? p.retstat->size() : 0;

        for (auto& statement : p.statements)
        {
            returns = std::max(std::visit(*this, statement.inner), returns);
        }
        return returns;
    } // namespace lua2wasm
};

struct compiler : utils
{
    std::vector<std::string> vars;
    std::vector<size_t> blocks;
    std::vector<size_t> functions;

    std::vector<std::vector<size_t>> upvalues;

    size_t function_name = 0;
    size_t data_name     = 0;

    size_t local_offset(size_t index) const
    {
        return index - functions.back();
    }

    size_t local_offset() const
    {
        return local_offset(vars.size());
    }

    bool is_index_local(size_t index) const
    {
        return index >= functions.back();
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
        auto exp = BinaryenLocalGet(mod, upvalue_index, ref_array_type());
        exp      = BinaryenArrayGet(mod, exp, const_i32(upvalues.back().size()), BinaryenTypeAnyref(), false);
        upvalues.back().push_back(index);
        return exp;
    }

    BinaryenExpressionRef get_var(const name_t& name)
    {
        auto [type, index] = find(name);
        switch (type)
        {
        case var_type::local:
            return BinaryenLocalGet(mod, index, BinaryenTypeAnyref());
        case var_type::upvalue:
            return get_upvalue(index);
        case var_type::global:
            return null();
        default:
            return BinaryenUnreachable(mod);
        }
    }

    BinaryenExpressionRef _funchead(const funchead& p)
    {
        return std::visit(overload{
                              [&](const name_t& name)
                              {
                                  return get_var(name);
                              },
                              [&](const expression& expr)
                              {
                                  return (*this)(expr);
                              },
                          },
                          p);
    }

    BinaryenExpressionRef _functail(const functail& p, BinaryenExpressionRef function)
    {
        auto args     = (*this)(p.args);
        auto exp      = BinaryenRefCast(mod, function, type<value_types::function>());
        auto func_ref = BinaryenStructGet(mod, 0, exp, BinaryenTypeFuncref(), false);
        auto upvalues = BinaryenStructGet(mod, 1, exp, ref_array_type(), false);

        BinaryenExpressionRef real_args[] = {
            upvalues,
            args,
        };

        exp = BinaryenCallRef(mod, func_ref, std::data(real_args), std::size(real_args), BinaryenTypeNone(), false);
        return exp;
    }

    BinaryenExpressionRef _vartail(const vartail& p, BinaryenExpressionRef var)
    {
        // todo
        return var;
    }

    BinaryenExpressionRef _varhead(const varhead& v)
    {
        return std::visit(overload{
                              [&](const std::pair<expression, vartail>& exp)
                              {
                                  return _vartail(exp.second, (*this)(exp.first));
                              },
                              [&](const name_t& name)
                              {
                                  return get_var(name);
                              },
                          },
                          v);
    }

    auto operator()(const assignments& p)
    {
        std::vector<BinaryenExpressionRef> result;
        for (auto& var : p.varlist)
        {
            auto exp = _varhead(var.head);
            for (auto& [func, var] : var.tail)
            {
                for (auto& f : func)
                    exp = _functail(f, exp);
                exp = _vartail(var, exp);
            }
        }
        (*this)(p.explist);
        return result;
    }
    auto operator()(const function_call& p)
    {
        auto exp = _funchead(p.head);
        for (auto& [vartails, functail] : p.tail)
        {
            for (auto& var : vartails)
            {
                exp = _vartail(var, exp);
            }

            exp = _functail(functail, exp);
        }
        return std::vector<BinaryenExpressionRef>{BinaryenDrop(mod, exp)};
    }
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
    auto operator()(const local_function& p) -> std::vector<BinaryenExpressionRef>
    {
        size_t offset = local_offset();

        vars.emplace_back(p.name);

        return {BinaryenLocalSet(mod, offset + 0, add_func_ref(p.name.c_str(), p.body))};
    }
    auto operator()(const local_variables& p)
    {
        auto explist = (*this)(p.explist);

        size_t offset = local_offset();
        size_t i      = 0;

        std::vector<BinaryenExpressionRef> result;
        //result.reserve(std::max(p.names.size(), explist.size()));

        //for (auto min = std::min(p.names.size(), explist.size()); i < min; ++i)
        //{
        //    vars.emplace_back(p.names[i]);
        //    auto exp = BinaryenLocalSet(mod, offset + i, explist[i]);
        //    result.push_back(exp);
        //}

        //while (i < p.names.size())
        //{
        //    vars.emplace_back(p.names[i]);
        //    i++;
        //}

        //while (i < p.explist.size())
        //{
        //    auto exp = BinaryenDrop(mod, explist[i]);
        //    result.push_back(exp);
        //    i++;
        //}
        return result;
    }

    BinaryenExpressionRef operator()(const expression& p)
    {
        return std::visit(*this, p.inner);
    }

    auto operator()(const expression_list& p) -> BinaryenExpressionRef
    {
        BinaryenExpressionRef exp = null();

        std::vector<BinaryenExpressionRef> result;
        for (auto& e : p)
        {
            exp       = (*this)(e);
            auto type = BinaryenExpressionGetType(exp);
            if (type == ref_array_type())
            {
                if (p.size() == 1)
                    return exp;

                ///BinaryenArrayGet(mod, exp, const_i32(0), BinaryenTypeAnyref(), false);
                //BinaryenSelect( BinaryenArrayGet(mod, exp, 0, BinaryenTypeAnyref(), false);
            }
            else
                result.push_back(new_value(exp));
        }

        exp = BinaryenArrayNewFixed(mod, BinaryenTypeGetHeapType(ref_array_type()), std::data(result), std::size(result));

        return exp;
    }

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

    auto add_string(const std::string& str)
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

    auto add_func(const char* name, const block& inner, const name_list& p, bool vararg) -> BinaryenFunctionRef
    {
        functions.push_back(vars.size());

        //for (auto& name : p)
        //    scope.emplace_back(name);
        vars.emplace_back(""); // upvalues
        vars.emplace_back(""); // args

        std::vector<const char*> names = {"*none"};

        for (auto& arg : p)
        {
            vars.emplace_back(arg);
            names.push_back(arg.c_str());
        }
        auto body = (*this)(inner);
        if (!p.empty())
        {
            BinaryenExpressionRef exp[2] = {
                BinaryenSwitch(mod, std::data(names), std::size(names) - 1, names.back(), BinaryenArrayLen(mod, BinaryenLocalGet(mod, args_index, ref_array_type())), nullptr),

            };
            size_t j = p.size();

            for (auto iter = p.rbegin(); iter != p.rend(); ++iter)
            {
                j--;
                exp[0] = BinaryenBlock(mod, iter->c_str(), std::data(exp), iter == p.rbegin() ? 1 : std::size(exp), BinaryenTypeAuto());
                exp[1] = BinaryenLocalSet(mod,
                                          j + 2,
                                          BinaryenArrayGet(mod,
                                                           BinaryenLocalGet(mod, args_index, ref_array_type()),
                                                           const_i32(j),
                                                           BinaryenTypeAnyref(),
                                                           false));
            }

            body.insert(body.begin(), BinaryenBlock(mod, names[0], std::data(exp), std::size(exp), BinaryenTypeAuto()));
        }
        body.push_back(BinaryenReturn(mod, null()));

        std::vector<BinaryenType> locals(count_locals{}(inner) + p.size(), BinaryenTypeAnyref());
        //std::vector<BinaryenType> params(p.size(), BinaryenTypeAnyref());
        //std::vector<BinaryenType> returns(max_returns{}(inner), BinaryenTypeAnyref());

        auto result = BinaryenAddFunctionWithHeapType(mod,
                                                      name,
                                                      BinaryenTypeGetHeapType(lua_func()),
                                                      locals.data(),
                                                      locals.size(),
                                                      BinaryenBlock(mod, nullptr, body.data(), body.size(), BinaryenTypeAuto()));

        BinaryenFunctionSetLocalName(result, args_index, "args");
        BinaryenFunctionSetLocalName(result, upvalue_index, "upvalues");

        size_t i = 2;
        for (auto& arg : p)
        {
            BinaryenFunctionSetLocalName(result, i++, arg.c_str());
        }

        //for (size_t i = functions.back() + 2; i < vars.size(); ++i)
        //    BinaryenFunctionSetLocalName(result, i - functions.back(), vars[i].c_str());

        vars.resize(functions.back());
        functions.pop_back();

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
                ups.push_back(BinaryenLocalGet(mod, local_offset(index), BinaryenTypeAnyref()));
            }
            else // upvalue
            {
                ups.push_back(get_upvalue(index));
            }
        }
        BinaryenExpressionRef exp[] = {BinaryenRefFunc(mod, name, sig), ups.empty() ? null() : BinaryenArrayNewFixed(mod, BinaryenTypeGetHeapType(ref_array_type()), std::data(ups), std::size(ups))};
        return BinaryenStructNew(mod, std::data(exp), std::size(exp), BinaryenTypeGetHeapType(type<value_types::function>()));
    }

    auto operator()(const function_body& p)
    {
        return add_func_ref(std::to_string(function_name++).c_str(), p);
    }

    auto operator()(const prefixexp& p)
    {
        auto exp = _funchead(p.chead);
        for (auto& tail : p.tail)
        {
            exp = std::visit(overload{
                                 [&](const functail& f)
                                 {
                                     return _functail(f, exp);
                                 },
                                 [&](const vartail& v)
                                 {
                                     return _vartail(v, exp);
                                 },
                             },
                             tail);
        }
        return exp;
    }

    template<typename T>
    auto operator()(const box<T>& p)
    {
        return (*this)(*p);
    }

    auto operator()(const table_constructor& p)
    {
        std::vector<BinaryenExpressionRef> exp;
        for (auto& field : p)
        {
            std::visit(overload{
                           [&](const std::monostate&)
                           {
                               exp.push_back(new_value((*this)(field.value)));
                           },
                           [&](const expression& index)
                           {
                               exp.push_back(new_value((*this)(index)));
                               exp.push_back(new_value((*this)(field.value)));
                           },
                           [&](const name_t& name)
                           {
                               exp.push_back(add_string(name));
                               exp.push_back(new_value((*this)(field.value)));
                           },
                       },
                       field.index);
        }

        BinaryenExpressionRef table_init[] = {

            BinaryenArrayNewFixed(mod, BinaryenTypeGetHeapType(ref_array_type()), std::data(exp), std::size(exp)),
        };
        return BinaryenStructNew(mod, std::data(table_init), std::size(table_init), BinaryenTypeGetHeapType(type<value_types::table>()));
    }
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
        BinaryenAddFunctionImport(mod, "print_integer", "print", "value", integer_type(), BinaryenTypeNone());
        BinaryenAddFunctionImport(mod, "print_number", "print", "value", number_type(), BinaryenTypeNone());
        BinaryenAddFunctionImport(mod, "print_nil", "print", "value", BinaryenTypeNullref(), BinaryenTypeNone());

        BinaryenExpressionRef args[] = {null(), null()};
        auto exp                     = BinaryenCall(mod, "*init", std::data(args), std::size(args), ref_array_type());
        exp                          = BinaryenArrayGet(mod, exp, const_i32(0), BinaryenTypeAnyref(), false);
        exp                          = BinaryenLocalSet(mod, 0, exp);

        std::vector<BinaryenExpressionRef> body;
        body.push_back(exp);
        std::tuple<const char*, BinaryenType, BinaryenExpressionRef (*)(BinaryenModuleRef, BinaryenExpressionRef)> casts[] = {
            {"print_number", type<value_types::number>(), [](BinaryenModuleRef mod, BinaryenExpressionRef exp)
             {
                 return BinaryenStructGet(mod, 0, exp, number_type(), false);
             }},
            {"print_integer", type<value_types::integer>(), [](BinaryenModuleRef mod, BinaryenExpressionRef exp)
             {
                 return BinaryenStructGet(mod, 0, exp, integer_type(), false);
             }},
        };

        exp = BinaryenBrOn(mod, BinaryenBrOnNull(), "nil", BinaryenLocalGet(mod, 0, BinaryenTypeAnyref()), BinaryenTypeNone());

        for (auto& c : casts)
        {
            exp = BinaryenBrOn(mod, BinaryenBrOnCast(), std::get<0>(c), exp, std::get<1>(c));
        }

        BinaryenExpressionRef block[] = {
            BinaryenDrop(mod, exp),
            BinaryenUnreachable(mod),
        };
        bool once = false;
        for (auto& c : casts)
        {
            exp  = BinaryenBlock(mod, std::get<0>(c), once ? &exp : std::data(block), once ? 1 : std::size(block), BinaryenTypeAuto());
            exp  = std::get<2>(c)(mod, exp);
            exp  = BinaryenReturnCall(mod, std::get<0>(c), &exp, 1, BinaryenTypeNone());
            once = true;
        }

        exp = BinaryenBlock(mod, "nil", &exp, 1, BinaryenTypeAuto());

        body.push_back(exp);
        exp = null();
        body.push_back(BinaryenReturnCall(mod, "print_nil", &exp, 1, BinaryenTypeNone()));

        BinaryenType locals[] = {BinaryenTypeAnyref()};
        return BinaryenAddFunction(mod,
                                   "convert",
                                   BinaryenTypeNone(),
                                   BinaryenTypeNone(),
                                   std::data(locals),
                                   std::size(locals),
                                   BinaryenBlock(mod, nullptr, body.data(), body.size(), BinaryenTypeAuto()));
    }
};

wasm::mod compile(const block& chunk)
{
    wasm::mod result;
    BinaryenModuleRef mod = reinterpret_cast<BinaryenModuleRef>(result.impl.get());
    compiler c{mod};
    c.build_types();
    auto start = c.add_func("*init", chunk, {}, true);
    c.convert();
    //BinaryenSetStart(mod, v);
    BinaryenAddFunctionExport(mod, "convert", "start");
    //BinaryenModuleAutoDrop(mod);
    BinaryenModuleValidate(mod);
    //BinaryenModuleInterpret(mod);
    //BinaryenModuleOptimize(mod);
    //BinaryenModuleSetTypeName(mod, builtHeapTypes[0], "SomeStruct");
    //BinaryenModuleSetFieldName(mod, builtHeapTypes[0], 0, "SomeField");
    return result;
}

} // namespace lua2wasm
