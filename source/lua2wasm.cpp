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

    BinaryenType types[10];

    using scope = std::vector<std::string>;
    std::vector<std::vector<scope>> env;

    size_t local_offset() const
    {
        size_t i = 0;
        for (auto& scope : env.back())
            i += scope.size();
        return i;
    }

    template<value_types T>
    BinaryenType type() const
    {
        return types[static_cast<std::underlying_type_t<value_types>>(T)];
    }

    void build_types()
    {
        BinaryenModuleSetFeatures(
            mod,
            BinaryenModuleGetFeatures(mod)
                | BinaryenFeatureGC()
                | BinaryenFeatureBulkMemory()
                | BinaryenFeatureReferenceTypes());

        struct struct_def
        {
            std::string type_name;
            std::vector<BinaryenType> field_types;
            std::vector<BinaryenPackedType> field_packs;
            std::vector<uint8_t> field_mut; // classic c++
            bool is_array = false;
        };

        struct_def defs[] = {
            {
                "nil",
                {},
                {},
                {},
            },
            {
                "boolean",
                {BinaryenTypeInt32()},
                {BinaryenPackedTypeNotPacked()},
                {false},
            },
            {
                "integer",
                {BinaryenTypeInt64()},
                {BinaryenPackedTypeNotPacked()},
                {false},
            },
            {
                "number",
                {BinaryenTypeFloat64()},
                {BinaryenPackedTypeNotPacked()},
                {false},
            },
            {
                "string",
                {BinaryenTypeInt32()},
                {BinaryenPackedTypeInt8()},
                {false},
                true,
            },
            {
                "function",
                {BinaryenTypeFuncref()},
                {BinaryenPackedTypeNotPacked()},
                {false},
            },
            {
                "userdata",
                {BinaryenTypeFuncref()},
                {BinaryenPackedTypeNotPacked()},
                {false},
            },
            {
                "thread",
                {BinaryenTypeFuncref()},
                {BinaryenPackedTypeNotPacked()},
                {false},
            },
            {
                "table",
                {BinaryenTypeFuncref()},
                {BinaryenPackedTypeNotPacked()},
                {false},
            },
        };
        TypeBuilderRef tb = TypeBuilderCreate(std::size(defs));

        for (size_t i = 0; i < std::size(defs); ++i)
        {
            auto& def = defs[i];
            if (def.is_array)
                TypeBuilderSetArrayType(
                    tb,
                    i,
                    def.field_types[0],
                    def.field_packs[0],
                    def.field_mut[0]);
            else
                TypeBuilderSetStructType(
                    tb,
                    i,
                    def.field_types.data(),
                    def.field_packs.data(),
                    reinterpret_cast<bool*>(def.field_mut.data()),
                    def.field_types.size());
        }

        BinaryenHeapType heap_types[std::size(defs)];

        if (!TypeBuilderBuildAndDispose(tb, (BinaryenHeapType*)&heap_types, nullptr, nullptr))
        {
            // error
        }
        for (size_t i = 0; i < std::size(defs); ++i)
        {
            auto& def = defs[i];

            types[i] = BinaryenTypeFromHeapType(heap_types[i], true);
            BinaryenModuleSetTypeName(mod, heap_types[i], def.type_name.c_str());
        }
    }

    BinaryenExpressionRef new_number(BinaryenExpressionRef num)
    {
        return BinaryenStructNew(mod, &num, 1, BinaryenTypeGetHeapType(type<value_types::number>()));
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

        std::visit(overload{
                       [](const name_t& name) {

                       },
                       [](const expression& expr) {
                       },
                   },
                   p.head);
        for (auto& [vartails, functail] : p.tail)
        {
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
    auto operator()(const function_definition& p)
    {
        add_func(p.function_name.back().c_str(), p.body.inner, p.body.params, p.body.vararg);
        std::vector<BinaryenExpressionRef> result;
        return result;
    }
    auto operator()(const local_function& p)
    {
        size_t offset = local_offset();

        env.back().back().emplace_back(p.name);

        std::vector<BinaryenExpressionRef> result;
        add_func(p.name.c_str(), p.body.inner, p.body.params, p.body.vararg);

        //exp = BinaryenLocalSet(mod, offset + 0, exp);
        return result;
    }
    auto operator()(const local_variables& p)
    {
        auto explist = (*this)(p.explist);
        auto& scope  = env.back().back();

        size_t offset = local_offset();
        size_t i      = 0;

        std::vector<BinaryenExpressionRef> result;
        result.reserve(std::max(p.names.size(), explist.size()));

        for (auto min = std::min(p.names.size(), explist.size()); i < min; ++i)
        {
            scope.emplace_back(p.names[i]);
            auto exp = new_number(explist[i]);
            exp      = BinaryenLocalSet(mod, offset + i, exp);
            result.push_back(exp);
        }

        while (i < p.names.size())
        {
            scope.emplace_back(p.names[i]);
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
        p.value ? "true" : "false";
        return BinaryenUnreachable(mod);
    }

    auto operator()(const int_type& p)
    {
        return BinaryenConst(mod, BinaryenLiteralInt64(p));
    }

    auto operator()(const float_type& p)
    {
        return BinaryenConst(mod, BinaryenLiteralFloat64(p));
    }

    auto operator()(const literal& p)
    {
        static int i = 0;
        auto name    = std::to_string(i++);

        BinaryenAddDataSegment(mod, name.c_str(), "", true, 0, p.str.data(), p.str.size());

        return BinaryenArrayNewData(mod, BinaryenTypeGetHeapType(type<value_types::string>()), name.c_str(), BinaryenConst(mod, BinaryenLiteralInt32(0)), BinaryenConst(mod, BinaryenLiteralInt32(p.str.size())));
    }

    auto operator()(const ellipsis& p)
    {
        return BinaryenUnreachable(mod);
    }

    auto add_func(const char* name, const block& inner, const name_list& p, bool vararg) -> BinaryenFunctionRef
    {
        auto& scope = env.emplace_back().emplace_back();

        for (auto& name : p)
            scope.emplace_back(name);

        BinaryenExpressionRef body = (*this)(inner);

        std::vector<BinaryenType> vars(count_locals{}(inner), BinaryenTypeAnyref());
        std::vector<BinaryenType> params(p.size(), BinaryenTypeAnyref());
        std::vector<BinaryenType> returns(max_returns{}(inner), BinaryenTypeAnyref());

        env.pop_back();
        return BinaryenAddFunction(mod, name, BinaryenTypeCreate(params.data(), params.size()), BinaryenTypeCreate(returns.data(), returns.size()), vars.data(), vars.size(), body);
    }

    auto operator()(const function_body& p)
    {
        auto func = add_func("a", p.inner, p.params, p.vararg);
        return BinaryenUnreachable(mod);
    }

    auto operator()(const box<prefixexp>& p)
    {
        return BinaryenUnreachable(mod);
    }
    auto operator()(const table_constructor& p)
    {
        return BinaryenUnreachable(mod);
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

    BinaryenExpressionRef operator()(const block& p)
    {
        env.back().emplace_back();
        std::vector<BinaryenExpressionRef> result;
        for (auto& statement : p.statements)
        {
            auto temp = std::visit(*this, statement.inner);
            result.insert(result.end(), temp.begin(), temp.end());
        }
        if (p.retstat)
        {
            auto list = (*this)(*p.retstat);

            //for (auto& l : list)
            //    l = new_number(l);
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

        env.back().pop_back();

        return BinaryenBlock(mod, nullptr, result.data(), result.size(), BinaryenTypeAuto());
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