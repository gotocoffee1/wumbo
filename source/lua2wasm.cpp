#include "lua2wasm.hpp"

#include <stack>
#include <unordered_map>

#include "util.hpp"

#include "binaryen-c.h"

namespace lua2wasm
{
using namespace ast;
using namespace wasm;

enum class value_types
{
    nil,
    boolean,
    integer,
    number,
    string,
    function,
    userdata,
    thread,
    table,
    dynamic,
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

struct compiler
{
    BinaryenModuleRef mod;

    BinaryenType types[11];

    std::vector<std::string> vars;
    std::vector<size_t> blocks;
    std::vector<size_t> functions;

    size_t function_name = 0;
    size_t data_name     = 0;

    size_t local_offset() const
    {
        return vars.size() - functions.back();
    }

    size_t find(const std::string& var_name)
    {
        if (auto local = std::find(vars.rbegin(), vars.rend(), var_name); local != vars.rend())
        {
            return std::distance(vars.begin(), std::next(local).base()) - functions.back();
        }
        return 0;
    }

    template<value_types T>
    BinaryenType type() const
    {
        return types[static_cast<std::underlying_type_t<value_types>>(T) + 2];
    }

    BinaryenType ref_array_type() const
    {
        return types[0];
    }

    BinaryenType lua_func() const
    {
        return types[1];
    }

    BinaryenType pair_list() const
    {
        return types[2];
    }

    void build_types()
    {
        BinaryenModuleSetFeatures(
            mod,
            BinaryenModuleGetFeatures(mod)
                | BinaryenFeatureGC()
                | BinaryenFeatureBulkMemory()
                | BinaryenFeatureReferenceTypes());

        enum class type
        {
            struct_,
            array_,
            signature,
        };

        struct struct_def
        {
            std::vector<BinaryenType> field_types;
            std::vector<BinaryenPackedType> field_packs;
            std::vector<uint8_t> field_muts; // classic c++
        };

        struct array_def
        {
            BinaryenType field_type;
            BinaryenPackedType field_pack;
            bool field_mut;
        };

        struct sig_def
        {
            std::vector<BinaryenType> param_types;
            std::vector<BinaryenType> return_types;
        };
        struct def
        {
            std::string type_name;
            std::variant<struct_def, array_def, sig_def> inner;
            bool nullable = true;
        };

        BinaryenType pair[] = {
            BinaryenTypeAnyref(),
            BinaryenTypeAnyref(),
        };

        def defs[] = {
            {
                "ref_array",
                array_def{
                    BinaryenTypeAnyref(),
                    BinaryenPackedTypeNotPacked(),
                    false,
                },
            },
            {
                "lua_function",
                sig_def{
                    {},
                    {},
                },
            },
            {
                "pair_list",
                array_def{
                    BinaryenTypeCreate(std::data(pair), std::size(pair)),
                    BinaryenPackedTypeNotPacked(),
                    false,
                },
            },
            {
                "boolean",
                struct_def{
                    {BinaryenTypeInt32()},
                    {BinaryenPackedTypeNotPacked()},
                    {false},
                },
            },
            {
                "integer",
                struct_def{
                    {BinaryenTypeInt64()},
                    {BinaryenPackedTypeNotPacked()},
                    {false},
                },
            },
            {
                "number",
                struct_def{
                    {BinaryenTypeFloat64()},
                    {BinaryenPackedTypeNotPacked()},
                    {false},
                },
            },
            {
                "string",
                array_def{
                    BinaryenTypeInt32(),
                    BinaryenPackedTypeInt8(),
                    false,

                },
            },
            {
                "function",
                struct_def{
                    {BinaryenTypeNone(), BinaryenTypeNone()},
                    {BinaryenPackedTypeNotPacked(), BinaryenPackedTypeNotPacked()},
                    {false, false},
                },
            },
            {
                "userdata",
                struct_def{
                    {BinaryenTypeFloat32()},
                    {BinaryenPackedTypeNotPacked()},
                    {false},
                },
            },
            {
                "thread",
                struct_def{
                    {BinaryenTypeFloat32(), BinaryenTypeFloat32()},
                    {BinaryenPackedTypeNotPacked(), BinaryenPackedTypeNotPacked()},
                    {false, false},
                },
            },
            {
                "table",
                struct_def{
                    {BinaryenTypeNone()},
                    {BinaryenPackedTypeNotPacked()},
                    {false},
                },
            },
        };

        TypeBuilderRef tb = TypeBuilderCreate(std::size(defs));

        BinaryenType ref_array = TypeBuilderGetTempRefType(tb, TypeBuilderGetTempHeapType(tb, 0), true);

        std::get<sig_def>(defs[1].inner).param_types.assign(2, ref_array);
        std::get<sig_def>(defs[1].inner).return_types.assign(1, ref_array);

        BinaryenType lua_function = TypeBuilderGetTempRefType(tb, TypeBuilderGetTempHeapType(tb, 1), true);

        std::get<struct_def>(defs[7].inner).field_types[0] = lua_function;
        std::get<struct_def>(defs[7].inner).field_types[1] = ref_array;

        std::get<struct_def>(defs[10].inner).field_types[0] = TypeBuilderGetTempRefType(tb, TypeBuilderGetTempHeapType(tb, 0), true);
        //std::get<struct_def>(defs[10].inner).field_types[0] = TypeBuilderGetTempRefType(tb, TypeBuilderGetTempHeapType(tb, 2), true);

        for (size_t i = 0; i < std::size(defs); ++i)
        {
            std::visit(overload{
                           [&](array_def& def)
                           {
                               TypeBuilderSetArrayType(
                                   tb,
                                   i,
                                   def.field_type,
                                   def.field_pack,
                                   def.field_mut);
                           },
                           [&](struct_def& def)
                           {
                               TypeBuilderSetStructType(
                                   tb,
                                   i,
                                   def.field_types.data(),
                                   def.field_packs.data(),
                                   reinterpret_cast<bool*>(def.field_muts.data()),
                                   def.field_types.size());
                           },
                           [&](sig_def& def)
                           {
                               auto& [params, returns] = def;
                               TypeBuilderSetSignatureType(tb, i, TypeBuilderGetTempTupleType(tb, params.data(), params.size()), TypeBuilderGetTempTupleType(tb, returns.data(), returns.size()));

                               //TypeBuilderSetSignatureType(tb, i, BinaryenTypeCreate(params.data(), params.size()), BinaryenTypeCreate(returns.data(), returns.size()));
                           },
                       },
                       defs[i].inner);
        }

        BinaryenHeapType heap_types[std::size(defs)];

        BinaryenIndex error_index;
        TypeBuilderErrorReason error_reason;
        if (!TypeBuilderBuildAndDispose(tb, (BinaryenHeapType*)&heap_types, &error_index, &error_reason))
        {
            throw;
        }
        for (size_t i = 0; i < std::size(defs); ++i)
        {
            auto& def = defs[i];

            types[i] = BinaryenTypeFromHeapType(heap_types[i], def.nullable);
            BinaryenModuleSetTypeName(mod, heap_types[i], def.type_name.c_str());
        }
    }

    BinaryenExpressionRef new_number(BinaryenExpressionRef num)
    {
        return BinaryenStructNew(mod, &num, 1, BinaryenTypeGetHeapType(type<value_types::number>()));
    }

    BinaryenExpressionRef new_integer(BinaryenExpressionRef num)
    {
        return BinaryenStructNew(mod, &num, 1, BinaryenTypeGetHeapType(type<value_types::integer>()));
    }

    BinaryenExpressionRef new_value(BinaryenExpressionRef exp)
    {
        auto type = BinaryenExpressionGetType(exp);
        if (type == BinaryenTypeFloat64())
            return new_number(exp);
        else if (type == BinaryenTypeInt64())
            return new_integer(exp);
        else
        {
            return exp;
        }
    }

    auto operator()(const assignments& p)
    {
        (*this)(p.explist);
        std::vector<BinaryenExpressionRef> result;
        return result;
    }
    auto operator()(const function_call& p)
    {
        std::vector<BinaryenExpressionRef> result;
        size_t local_index;
        std::visit(overload{
                       [&](const name_t& name)
                       {
                           local_index = find(name);
                       },
                       [](const expression& expr) {
                       },
                   },
                   p.head);
        for (auto& [vartails, functail] : p.tail)
        {
            auto args     = (*this)(functail.args);
            auto exp      = BinaryenLocalGet(mod, local_index, BinaryenTypeAnyref());
            exp           = BinaryenRefCast(mod, exp, type<value_types::function>());
            auto func_ref = BinaryenStructGet(mod, 0, exp, BinaryenTypeFuncref(), false);
            auto upvalues = BinaryenStructGet(mod, 1, exp, ref_array_type(), false);

            BinaryenExpressionRef real_args[] = {
                BinaryenArrayNewFixed(mod, BinaryenTypeGetHeapType(ref_array_type()), args.data(), args.size()),
                upvalues,
            };

            exp = BinaryenDrop(mod, BinaryenCallRef(mod, func_ref, std::data(real_args), std::size(real_args), BinaryenTypeNone(), false));
            result.push_back(exp);
        }
        return result;
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
        result.reserve(std::max(p.names.size(), explist.size()));

        for (auto min = std::min(p.names.size(), explist.size()); i < min; ++i)
        {
            vars.emplace_back(p.names[i]);
            auto exp = new_value(explist[i]);
            exp      = BinaryenLocalSet(mod, offset + i, exp);
            result.push_back(exp);
        }

        while (i < p.names.size())
        {
            vars.emplace_back(p.names[i]);
            i++;
        }

        while (i < p.explist.size())
        {
            auto exp = BinaryenDrop(mod, explist[i]);
            result.push_back(exp);
            i++;
        }
        return result;
    }

    BinaryenExpressionRef operator()(const expression& p)
    {
        return std::visit(*this, p.inner);
    }

    auto operator()(const expression_list& p) -> std::vector<BinaryenExpressionRef>
    {
        std::vector<BinaryenExpressionRef> list;
        for (auto& exp : p)
        {
            list.push_back((*this)(exp));
        }
        return list;
    }

    auto operator()(const nil&)
    {
        return BinaryenUnreachable(mod);
    }

    auto operator()(const boolean& p)
    {
        return BinaryenConst(mod, BinaryenLiteralInt32(p.value));
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
        return BinaryenArrayNewData(mod, BinaryenTypeGetHeapType(type<value_types::string>()), name.c_str(), BinaryenConst(mod, BinaryenLiteralInt32(0)), BinaryenConst(mod, BinaryenLiteralInt32(str.size())));
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
        vars.emplace_back(""); // args
        vars.emplace_back(""); // upvalues

        auto body = (*this)(inner);

        body.push_back(BinaryenReturn(mod, BinaryenRefNull(mod, BinaryenTypeNullref())));

        std::vector<BinaryenType> locals(count_locals{}(inner), BinaryenTypeAnyref());
        //std::vector<BinaryenType> params(p.size(), BinaryenTypeAnyref());
        //std::vector<BinaryenType> returns(max_returns{}(inner), BinaryenTypeAnyref());

        auto result = BinaryenAddFunctionWithHeapType(mod,
                                                      name,
                                                      BinaryenTypeGetHeapType(lua_func()),
                                                      locals.data(),
                                                      locals.size(),
                                                      BinaryenBlock(mod, nullptr, body.data(), body.size(), BinaryenTypeAuto()));

        BinaryenFunctionSetLocalName(result, 0, "args");
        BinaryenFunctionSetLocalName(result, 1, "upvalues");

        //for (size_t i = functions.back() + 2; i < vars.size(); ++i)
        //    BinaryenFunctionSetLocalName(result, i - functions.back(), vars[i].c_str());

        vars.resize(functions.back());
        functions.pop_back();

        return result;
    }

    auto add_func_ref(const char* name, const function_body& p) -> BinaryenExpressionRef
    {
        auto func = add_func(name, p.inner, p.params, p.vararg);

        auto sig                    = BinaryenTypeFromHeapType(BinaryenFunctionGetType(func), false);
        BinaryenExpressionRef exp[] = {BinaryenRefFunc(mod, name, sig), BinaryenRefNull(mod, BinaryenTypeNullref())};
        return BinaryenStructNew(mod, std::data(exp), std::size(exp), BinaryenTypeGetHeapType(type<value_types::function>()));
    }

    auto operator()(const function_body& p)
    {
        return add_func_ref(std::to_string(function_name++).c_str(), p);
    }

    auto operator()(const box<prefixexp>& p)
    {
        return BinaryenUnreachable(mod);
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
    auto operator()(const box<bin_operation>& p)
    {
        auto lhs_type = get_return_type(p->lhs);
        auto rhs_type = get_return_type(p->rhs);
        BinaryenExpressionRef result;

        auto lhs = (*this)(p->lhs);
        auto rhs = (*this)(p->rhs);

        BinaryenOp op;

        if (lhs_type == value_types::integer && rhs_type == value_types::integer)
        {
            switch (p->op)
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
            switch (p->op)
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

    auto operator()(const box<un_operation>& p)
    {
        auto rhs_type = get_return_type(p->rhs);

        auto rhs = (*this)(p->rhs);
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

            for (auto& l : list)
                l = new_value(l);
            if (list.empty())
                result.push_back(BinaryenReturn(mod, nullptr));
            else if (list.size() == 1)
                result.push_back(BinaryenReturn(mod, list[0]));
            else
            {
                auto ret = BinaryenTupleMake(mod, list.data(), list.size());
                result.push_back(BinaryenReturn(mod, ret));
            }
        }

        vars.resize(blocks.back());
        blocks.pop_back();
        return result;
    }
};

wasm::mod compile(const block& chunk)
{
    wasm::mod result;
    BinaryenModuleRef mod = reinterpret_cast<BinaryenModuleRef>(result.impl.get());
    compiler c{mod};
    c.build_types();

    auto start = c.add_func("start", chunk, {}, true);
    //BinaryenSetStart(mod, v);
    BinaryenAddFunctionExport(mod, "start", "start");
    BinaryenModuleValidate(mod);
    //BinaryenModuleInterpret(mod);
    //BinaryenModuleOptimize(mod);
    //BinaryenModuleSetTypeName(mod, builtHeapTypes[0], "SomeStruct");
    //BinaryenModuleSetFieldName(mod, builtHeapTypes[0], 0, "SomeField");
    return result;
}

} // namespace lua2wasm