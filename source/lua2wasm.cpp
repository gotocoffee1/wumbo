#include "lua2wasm.hpp"

#include <array>
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
        auto exp = local_get(upvalue_index, ref_array_type());
        exp      = BinaryenArrayGet(mod, exp, const_i32(upvalues.back().size()), anyref(), false);
        upvalues.back().push_back(index);
        return exp;
    }

    BinaryenExpressionRef get_var(const name_t& name)
    {
        auto [type, index] = find(name);
        switch (type)
        {
        case var_type::local:
            return local_get(index, anyref());
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
        auto t        = type<value_types::function>();
        auto local    = alloc_local(t);
        auto args     = (*this)(p.args);
        auto tee      = BinaryenLocalTee(mod, local, BinaryenRefCast(mod, function, t), t);
        auto func_ref = BinaryenStructGet(mod, 0, local_get(local, t), BinaryenTypeFuncref(), false);
        auto upvalues = BinaryenStructGet(mod, 1, tee, ref_array_type(), false);
        free_local(local);

        BinaryenExpressionRef real_args[] = {
            upvalues,
            args,
        };

        return BinaryenCallRef(mod, func_ref, std::data(real_args), std::size(real_args), BinaryenTypeNone(), false);
    }

    BinaryenExpressionRef _vartail(const vartail& p, BinaryenExpressionRef var)
    {
        return std::visit(overload{
                              [&](const expression& exp)
                              {
                                  return table_get(var, (*this)(exp));
                              },
                              [&](const name_t& name)
                              {
                                  //TODO
                                  //return table_get(var, exp);
                                  return null();
                              },
                          },
                          p);
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
    auto operator()(const local_variables& p) -> std::vector<BinaryenExpressionRef>
    {
        auto explist = (*this)(p.explist);

        return {unpack_locals(p.names, explist)};
    }

    BinaryenExpressionRef operator()(const expression& p)
    {
        return std::visit(*this, p.inner);
    }

    auto operator()(const expression_list& p) -> BinaryenExpressionRef
    {
        if (p.empty())
            return null();
        std::vector<BinaryenExpressionRef> result;
        size_t i = 0;
        for (auto& e : p)
        {
            i++;
            auto exp  = (*this)(e);
            auto type = BinaryenExpressionGetType(exp);
            if (type == ref_array_type())
            {
                if (p.size() == 1)
                    return exp;

                auto local = alloc_local(type);
                auto l_get = local_get(local, type);
                exp        = BinaryenLocalTee(mod, local, exp, type);
                if (i == p.size())
                {
                    auto new_array = alloc_local(type);

                    std::vector<BinaryenExpressionRef> res;

                    res.push_back(BinaryenArrayCopy(mod,
                                                    BinaryenLocalTee(mod,
                                                                     new_array,
                                                                     BinaryenArrayNew(mod,
                                                                                      BinaryenTypeGetHeapType(type),
                                                                                      BinaryenBinary(mod, BinaryenAddInt32(), const_i32(result.size()), BinaryenArrayLen(mod, exp)),
                                                                                      nullptr),
                                                                     type),
                                                    const_i32(result.size()),
                                                    l_get,
                                                    const_i32(0),
                                                    BinaryenArrayLen(mod, l_get)));
                    size_t j = 0;
                    for (auto& init : result)
                        res.push_back(BinaryenArraySet(mod, local_get(new_array, type), const_i32(j++), init));

                    res.push_back(local_get(new_array, type));

                    free_local(local);
                    free_local(new_array);
                    return BinaryenBlock(mod, "", std::data(res), std::size(res), type);
                }
                else
                {
                    exp = BinaryenSelect(mod,
                                         BinaryenArrayLen(mod, l_get),
                                         BinaryenArrayGet(mod,
                                                          exp,
                                                          const_i32(0),
                                                          anyref(),
                                                          false),
                                         null(),
                                         anyref());
                    result.push_back(exp);
                    free_local(local);
                }
            }
            else
                result.push_back(new_value(exp));
        }

        return BinaryenArrayNewFixed(mod, BinaryenTypeGetHeapType(ref_array_type()), std::data(result), std::size(result));
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

    BinaryenExpressionRef unpack_locals(const name_list& p, BinaryenExpressionRef list)
    {
        auto offset                    = local_offset();
        std::string none               = "+none" + std::to_string(label_name++);
        std::vector<const char*> names = {none.c_str()};
        for (auto& arg : p)
        {
            vars.emplace_back(arg);
            names.push_back(arg.c_str());
        }

        BinaryenExpressionRef exp[2] = {
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
        size_t j = p.size();

        for (auto iter = p.rbegin(); iter != p.rend(); ++iter)
        {
            j--;
            exp[0] = BinaryenBlock(mod, iter->c_str(), std::data(exp), iter == p.rbegin() ? 1 : std::size(exp), BinaryenTypeAuto());
            exp[1] = BinaryenLocalSet(mod,
                                      j + offset,
                                      BinaryenArrayGet(mod,
                                                       list,
                                                       const_i32(j),
                                                       anyref(),
                                                       false));
        }

        return BinaryenBlock(mod, names[0], std::data(exp), std::size(exp), BinaryenTypeAuto());
    }

    auto add_func(const char* name, const block& inner, const name_list& p, bool vararg) -> BinaryenFunctionRef
    {
        functions.push_back(vars.size());

        //for (auto& name : p)
        //    scope.emplace_back(name);
        //vars.emplace_back("+upvalues");
        //vars.emplace_back("+args");

        std::vector<BinaryenType> locals(count_locals{}(inner) + p.size(), anyref());
        std::get<2>(util_locals.emplace_back()) = locals.size();

        BinaryenExpressionRef init;
        if (!p.empty())
            init = unpack_locals(p, local_get(args_index, ref_array_type()));
        auto body = (*this)(inner);
        if (!p.empty())
            body.insert(body.begin(), init);

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
                ups.push_back(local_get(local_offset(index), anyref()));
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

    BinaryenExpressionRef calc_hash(BinaryenExpressionRef key)
    {
        return const_i32(3);
    }

    BinaryenExpressionRef find_bucket(BinaryenExpressionRef table, BinaryenExpressionRef hash)
    {
        auto hash_map = BinaryenStructGet(mod, tbl_hash_index, table, ref_array_type(), false);

        auto bucket_index = BinaryenBinary(mod, BinaryenRemUInt32(), hash, BinaryenArrayLen(mod, hash_map));
        auto bucket       = BinaryenArrayGet(mod, hash_map, bucket_index, ref_array_type(), false);

        return bucket;
    }

    BinaryenExpressionRef table_get(BinaryenExpressionRef table, BinaryenExpressionRef key)
    {
        BinaryenExpressionRef args[] = {
            table,
            key,
        };

        return BinaryenCall(mod, "*table_get", std::data(args), std::size(args), anyref());
    }

    BinaryenFunctionRef compare(const char* name, value_types vtype)
    {
        std::vector<std::tuple<const char*, value_types>> casts = {
            {
                name,
                vtype,
            },
        };
        auto exp = local_get(0, type(vtype));

        BinaryenType params[] = {
            type(vtype),
            anyref(),
        };

        return BinaryenAddFunction(mod,
                                   (std::string("*key_compare_") + name).c_str(),
                                   BinaryenTypeCreate(std::data(params), std::size(params)),
                                   BinaryenTypeInt32(),
                                   nullptr,
                                   0,
                                   make_block(switch_value(local_get(1, anyref()), casts, [&](value_types type_right, BinaryenExpressionRef exp_right)
                                                           {
                                                               if (vtype != type_right)
                                                               {
                                                                   if (!exp_right)
                                                                       return BinaryenUnreachable(mod);
                                                                   return const_i32(0);
                                                               }
                                                               switch (type_right)
                                                               {
                                                               case value_types::integer:
                                                                   return BinaryenReturn(mod, BinaryenBinary(mod, BinaryenEqInt64(), BinaryenStructGet(mod, 0, exp, integer_type(), false), BinaryenStructGet(mod, 0, exp_right, integer_type(), false)));
                                                               case value_types::number:
                                                                   return BinaryenReturn(mod, BinaryenBinary(mod, BinaryenEqFloat64(), BinaryenStructGet(mod, 0, exp, number_type(), false), BinaryenStructGet(mod, 0, exp_right, number_type(), false)));
                                                               case value_types::table:
                                                               case value_types::function:
                                                               case value_types::thread:
                                                                   return BinaryenRefEq(mod, exp, exp_right);
                                                               case value_types::nil:
                                                               case value_types::boolean:
                                                               case value_types::string:
                                                               case value_types::userdata:
                                                               case value_types::dynamic:
                                                               default:
                                                                   return BinaryenUnreachable(mod);
                                                               }
                                                           }),
                                              nullptr,
                                              BinaryenTypeInt32()));
    }

    BinaryenFunctionRef func_table_get()
    {
        //auto hash = calc_hash(key);

        //auto bucket = find_bucket(table, hash);

        //auto bucket = ;
        size_t i = 2;

        auto dec = [&](BinaryenExpressionRef first)
        {
            return BinaryenBinary(mod,
                                  BinaryenSubInt32(),
                                  first,
                                  const_i32(2));
        };
        //(BinaryenExpressionRef[]){            BinaryenLocalSet(mod, i, dec(BinaryenArrayLen(mod, bucket)))};
        auto table  = BinaryenLocalGet(mod, 0, anyref());
        auto key    = BinaryenLocalGet(mod, 1, anyref());
        table       = BinaryenRefCast(mod, table, type<value_types::table>());
        auto bucket = BinaryenStructGet(mod, tbl_hash_index, table, ref_array_type(), false);

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

        for (auto& [name, value] : casts)
            compare(name, value);

        auto result = switch_value(key,
                                   casts,
                                   [&](value_types type, BinaryenExpressionRef exp)
                                   {
                                       std::string loop = "+loop" + std::to_string(label_name++);

                                       BinaryenExpressionRef args[] = {
                                           exp,
                                           BinaryenArrayGet(mod, bucket, local_get(i, BinaryenTypeInt32()), anyref(), false),
                                       };
                                       const char* comparator;
                                       switch (type)
                                       {
                                       case value_types::integer:
                                           comparator = "*key_compare_integer";
                                           break;
                                       case value_types::number:
                                           comparator = "*key_compare_number";
                                           break;
                                       case value_types::nil:
                                       case value_types::boolean:
                                       case value_types::string:
                                       case value_types::function:
                                       case value_types::userdata:
                                       case value_types::thread:
                                       case value_types::table:

                                       default:
                                       case value_types::dynamic:
                                           return BinaryenUnreachable(mod);
                                       }

                                       return make_block(std::array{
                                           BinaryenLocalSet(mod, i, dec(BinaryenArrayLen(mod, bucket))),
                                           BinaryenLoop(mod,
                                                        loop.c_str(),
                                                        make_block(std::array{

                                                            BinaryenIf(mod,
                                                                       BinaryenCall(mod, comparator, std::data(args), std::size(args), BinaryenTypeInt32()),
                                                                       BinaryenReturn(mod, BinaryenArrayGet(mod, bucket, BinaryenBinary(mod, BinaryenAddInt32(), local_get(i, BinaryenTypeInt32()), const_i32(1)), anyref(), false)),
                                                                       nullptr),

                                                            BinaryenBreak(mod, loop.c_str(), BinaryenLocalTee(mod, i, dec(local_get(i, BinaryenTypeInt32())), BinaryenTypeInt32()), nullptr),
                                                        })),
                                           BinaryenReturn(mod, null()),
                                       });
                                   });

        BinaryenType params[] = {
            anyref(),
            anyref(),
        };

        BinaryenType locals[] = {
            BinaryenTypeInt32(),
        };
        return BinaryenAddFunction(mod,
                                   "*table_get",
                                   BinaryenTypeCreate(std::data(params), std::size(params)),
                                   anyref(),
                                   std::data(locals),
                                   std::size(locals),
                                   make_block(result));
    }

    auto operator()(const table_constructor& p)
    {
        std::vector<BinaryenExpressionRef> exp;
        expression_list array_init;
        for (auto& field : p)
        {
            std::visit(overload{
                           [&](const std::monostate&)
                           {
                               array_init.push_back(field.value);
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

        auto array = (*this)(array_init);

        BinaryenExpressionRef table_init[] = {

            array,
            //array,
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
