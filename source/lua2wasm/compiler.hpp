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
    struct local_var
    {
        std::string name;
        size_t name_offset;
        BinaryenType type;
        using flag_t = uint8_t;
        flag_t flags = 0;

        const char* current_name() const
        {
            return name.c_str() + (name.size() - name_offset);
        }

        static constexpr flag_t is_used   = 1 << 0;
        static constexpr flag_t is_helper = 1 << 2;
    };

    struct function_stack
    {
        // count nested loops per function
        std::vector<size_t> loop_stack;
        size_t loop_counter;

        std::vector<size_t> blocks;
        std::vector<std::tuple<size_t, size_t>> functions;
        std::vector<std::vector<size_t>> upvalues;

        std::vector<local_var> vars;

        void push_loop()
        {
            loop_stack.back()++;
            loop_counter++;
        }

        void pop_loop()
        {
            loop_stack.back()++;
            loop_counter++;
        }

        void push_block()
        {
            blocks.push_back(vars.size());
        }

        void pop_block()
        {
            for (size_t i = blocks.size(); i < vars.size(); ++i)
            {
                auto& var = vars[i];
                if (!(var.flags & local_var::is_helper))
                    var.flags &= local_var::is_used;
            }
            blocks.pop_back();
        }

        void push_function(size_t func_arg_count = func_arg_count)
        {
            upvalues.emplace_back();
            functions.emplace_back(vars.size(), func_arg_count);
            loop_stack.push_back(0);
        }

        void pop_function()
        {
            auto [func_offset, func_arg_count] = functions.back();

            loop_stack.pop_back();

            vars.resize(func_offset);
            functions.pop_back();
            upvalues.pop_back();
        }

        size_t local_offset(size_t index) const
        {
            auto [func_offset, func_arg_count] = functions.back();

            return func_arg_count + (index - func_offset);
        }

        size_t local_offset() const
        {
            return local_offset(vars.size());
        }

        bool is_index_local(size_t index) const
        {
            auto [func_offset, func_arg_count] = functions.back();

            return index >= func_offset;
        }

        void free_local(size_t pos)
        {
            auto [func_offset, func_arg_count] = functions.back();

            vars[(pos - func_arg_count) + func_offset].flags &= ~local_var::is_used;
        }

        size_t alloc_local(BinaryenType type, std::string_view name = "", bool helper = true)
        {
            auto [func_offset, func_arg_count] = functions.back();

            for (size_t i = func_offset; i < vars.size(); ++i)
            {
                auto& var = vars[i];
                if (type == var.type && !(var.flags & local_var::is_used))
                {
                    var.name += name;
                    var.name_offset = name.size();
                    var.flags |= local_var::is_used;
                    if (helper)
                        var.flags |= local_var::is_helper;
                    else
                        var.flags &= ~local_var::is_helper;

                    return local_offset(i);
                }
            }
            auto& var       = vars.emplace_back();
            var.name        = name;
            var.name_offset = name.size();
            var.flags       = local_var::is_used;
            var.type        = type;
            if (helper)
                var.flags |= local_var::is_helper;

            return local_offset(vars.size() - 1);
        }

        size_t alloc_lua_local(std::string_view name, BinaryenType type)
        {
            return alloc_local(type, name, false);
        }

        std::tuple<var_type, size_t> find(const std::string& var_name) const
        {
            if (auto local = std::find_if(vars.rbegin(), vars.rend(), [&var_name](const local_var& var)
                                          {
                                              return !(var.flags & local_var::is_helper) && var.current_name() == var_name;
                                          });
                local != vars.rend())
            {
                auto pos = std::distance(vars.begin(), std::next(local).base());
                if (is_index_local(pos)) // local
                    return {var_type::local, local_offset(pos)};
                else // upvalue
                    return {var_type::upvalue, pos};
            }
            return {var_type::global, 0}; // global
        }
    };

    struct help_var_scope
    {
        function_stack& _self;
        size_t index;
        help_var_scope(function_stack& self, BinaryenType type, std::string_view name = "")
            : _self{self}
        {
            index = _self.alloc_local(type, name, true);
        }

        ~help_var_scope()
        {
            _self.free_local(index);
        }

        operator size_t()
        {
            return index;
        }
    };

    struct loop_scope
    {
        function_stack& _self;
        loop_scope(function_stack& self)
            : _self{self}
        {
            _self.push_loop();
        }

        ~loop_scope()
        {
            _self.pop_loop();
        }
    };

    struct block_scope
    {
        function_stack& _self;
        block_scope(function_stack& self)
            : _self{self}
        {
            _self.push_block();
        }

        ~block_scope()
        {
            _self.pop_block();
        }
    };

    struct function_frame
    {
        function_stack& _self;

        function_frame(function_stack& self, size_t func_arg_count = func_arg_count)
            : _self{self}
        {
            _self.push_function(func_arg_count);
        }

        ~function_frame()
        {
            _self.pop_function();
        }

        std::vector<BinaryenType> get_local_type_list() const
        {
            auto [func_offset, func_arg_count] = _self.functions.back();

            std::vector<BinaryenType> locals;
            for (size_t i = func_offset; i < _self.vars.size(); ++i)
                locals.push_back(_self.vars[i].type);
            return locals;
        }

        void set_local_names(BinaryenFunctionRef func) const
        {
            auto [func_offset, func_arg_count] = _self.functions.back();

            for (size_t i = func_offset; i < _self.vars.size(); ++i)
            {
                if (!_self.vars[i].name.empty())
                    BinaryenFunctionSetLocalName(func, _self.local_offset(i), _self.vars[i].name.c_str());
            }
        }

        std::vector<size_t> get_requested_upvalues()
        {
            return std::move(_self.upvalues.back());
        }
    };

    function_stack _func_stack;

    size_t function_name = 0;
    size_t data_name     = 0;
    size_t label_name    = 0;

    BinaryenExpressionRef get_upvalue(size_t index)
    {
        auto& up = _func_stack.upvalues.back();
        int32_t result;
        if (auto iter = std::find(up.begin(), up.end(), index); iter != up.end())
        {
            result = std::distance(up.begin(), iter);
        }
        else
        {
            result = up.size();
            up.push_back(index);
        }
        auto exp = local_get(upvalue_index, upvalue_array_type());
        return array_get(exp, const_i32(result), upvalue_type());
    }

    BinaryenExpressionRef get_var(const name_t& name)
    {
        auto [var_type, index] = _func_stack.find(name);
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
        auto [var_type, index] = _func_stack.find(name);
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
    auto operator()(const goto_statement& p)
    {
        std::vector<BinaryenExpressionRef> result;
        return result;
    }
    auto to_bool()
    {
        std::vector<std::tuple<const char*, value_types>> casts = {
            {
                "boolean",
                value_types::boolean,
            },
        };

        return BinaryenAddFunction(mod,
                                   "*to_bool",
                                   anyref(),
                                   size_type(),
                                   nullptr,
                                   0,
                                   make_block(switch_value(local_get(0, anyref()), casts, [&](value_types type, BinaryenExpressionRef exp)
                                                           {
                                                               const char* func;
                                                               switch (type)
                                                               {
                                                               case value_types::nil:
                                                                   return BinaryenReturn(mod, const_i32(0));
                                                               case value_types::boolean:
                                                                   return BinaryenReturn(mod, BinaryenI31Get(mod, exp, false));
                                                               default:
                                                                   return BinaryenReturn(mod, const_i32(1));
                                                               }
                                                           })));
    }
    std::vector<BinaryenExpressionRef> operator()(const do_statement& p)
    {
        return (*this)(p.inner);
    }

    std::vector<BinaryenExpressionRef> operator()(const if_statement& p)
    {
        BinaryenExpressionRef res = nullptr;
        for (auto& [cond_exp, body] : p.cond_block)
        {
            auto cond = (*this)(cond_exp);
            auto temp = make_if(make_call("*to_bool", cond, size_type()), make_block((*this)(body)));
            if (res)
                BinaryenIfSetIfFalse(res, temp);
            res = temp;
        }
        if (p.else_block)
            BinaryenIfSetIfFalse(res, make_block((*this)(*p.else_block)));

        return {res};
    }

    std::vector<BinaryenExpressionRef> operator()(const for_statement& p);
    std::vector<BinaryenExpressionRef> operator()(const key_break& p);
    std::vector<BinaryenExpressionRef> operator()(const while_statement& p);
    std::vector<BinaryenExpressionRef> operator()(const repeat_statement& p);
    std::vector<BinaryenExpressionRef> operator()(const for_each& p);

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
        auto offset                    = _func_stack.local_offset();
        std::string none               = "+none" + std::to_string(label_name++);
        std::vector<const char*> names = {none.c_str()};
        bool is_upvalue                = true;
        std::vector<BinaryenExpressionRef> result;

        for (auto& arg : p)
        {
            auto index = _func_stack.alloc_lua_local(arg, upvalue_type());
            names.push_back(arg.c_str());

            if (is_upvalue)
                result.push_back(local_set(index,
                                           BinaryenStructNew(
                                               mod,
                                               nullptr,
                                               0,
                                               BinaryenTypeGetHeapType(upvalue_type()))));
        }
        std::array<BinaryenExpressionRef, 2> exp = {
            BinaryenSwitch(mod,
                           std::data(names),
                           std::size(names) - 1,
                           names.back(),
                           array_len(BinaryenBrOn(mod,
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

            auto get = array_get(
                list,
                const_i32(j),
                anyref());

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

    template<typename F>
    auto add_func(const char* name, const name_list& p, bool vararg, F&& f)
    {
        function_frame frame{_func_stack, func_arg_count};

        //std::vector<BinaryenType> locals(/* count_locals{}(inner) +*/ p.size(), upvalue_type());

        std::vector<BinaryenExpressionRef> body;
        if (!p.empty())
            body = unpack_locals(p, local_get(args_index, ref_array_type()));

        auto body_temp = f();

        body.insert(body.end(), body_temp.begin(), body_temp.end());

        body.push_back(BinaryenReturn(mod, null()));

        auto locals = frame.get_local_type_list();

        auto result = BinaryenAddFunctionWithHeapType(mod,
                                                      name,
                                                      BinaryenTypeGetHeapType(lua_func()),
                                                      std::data(locals),
                                                      std::size(locals),
                                                      make_block(body));

        BinaryenFunctionSetLocalName(result, args_index, "args");
        BinaryenFunctionSetLocalName(result, upvalue_index, "upvalues");

        frame.set_local_names(result);

        //for (size_t i = functions.back() + 2; i < vars.size(); ++i)
        //    BinaryenFunctionSetLocalName(result, i - functions.back(), vars[i].c_str());

        return std::tuple{result, frame.get_requested_upvalues()};
    }

    auto add_func(const char* name, const block& inner, const name_list& p, bool vararg)
    {
        return add_func(name, p, vararg, [&]()
                        {
                            return (*this)(inner);
                        });

        //functions.emplace_back(vars.size(), func_arg_count);
        //loop_stack.push_back(0);

        //for (auto& name : p)
        //    scope.emplace_back(name);
        //vars.emplace_back("+upvalues");
        //vars.emplace_back("+args");

        //std::vector<BinaryenType> locals(count_locals{}(inner) + p.size(), upvalue_type());

        //std::vector<BinaryenExpressionRef> body;
        //if (!p.empty())
        //    body = unpack_locals(p, local_get(args_index, ref_array_type()));
        //auto body_temp = (*this)(inner);

        //body.insert(body.end(), body_temp.begin(), body_temp.end());

        //body.push_back(BinaryenReturn(mod, null()));

        //auto& types = std::get<0>(util_locals.back());
        //locals.insert(locals.end(), types.begin(), types.end());

        //auto result = BinaryenAddFunctionWithHeapType(mod,
        //                                              name,
        //                                              BinaryenTypeGetHeapType(lua_func()),
        //                                              locals.data(),
        //                                              locals.size(),
        //                                              BinaryenBlock(mod, nullptr, body.data(), body.size(), BinaryenTypeAuto()));

        //BinaryenFunctionSetLocalName(result, args_index, "args");
        //BinaryenFunctionSetLocalName(result, upvalue_index, "upvalues");

        //size_t i = func_arg_count;
        //for (auto& arg : p)
        //{
        //    BinaryenFunctionSetLocalName(result, i++, arg.c_str());
        //}

        ////for (size_t i = functions.back() + 2; i < vars.size(); ++i)
        ////    BinaryenFunctionSetLocalName(result, i - functions.back(), vars[i].c_str());

        //loop_stack.pop_back();

        //        auto [func_offset, func_arg_count] = functions.back();

        //vars.resize(func_offset);
        //functions.pop_back();

        //return result;
    }

    auto add_func_ref(const char* name, const function_body& p) -> BinaryenExpressionRef
    {
        auto [func, req_ups] = add_func(name, p.inner, p.params, p.vararg);

        std::vector<BinaryenExpressionRef> ups;

        for (auto index : req_ups)
        {
            if (_func_stack.is_index_local(index)) // local
            {
                ups.push_back(local_get(_func_stack.local_offset(index), upvalue_type()));
            }
            else // upvalue
            {
                ups.push_back(get_upvalue(index));
            }
        }
        auto sig                    = BinaryenTypeFromHeapType(BinaryenFunctionGetType(func), false);
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

    void open_basic_lib();
    void setup_env();

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
        block_scope block{_func_stack};

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

        return result;
    }

    auto convert()
    {
        func_table_get();
        to_bool();
        BinaryenAddFunctionImport(mod, "print_integer", "print", "value", integer_type(), BinaryenTypeNone());
        BinaryenAddFunctionImport(mod, "print_number", "print", "value", number_type(), BinaryenTypeNone());
        BinaryenAddFunctionImport(mod, "print_string", "print", "string", BinaryenTypeExternref(), BinaryenTypeNone());
        BinaryenAddFunctionImport(mod, "print_nil", "print", "value", BinaryenTypeNullref(), BinaryenTypeNone());
        BinaryenAddFunctionImport(mod, "new_u8array", "print", "array", size_type(), BinaryenTypeExternref());

        {
            BinaryenType types[] = {
                BinaryenTypeExternref(),
                size_type(),
                size_type(),
            };
            BinaryenAddFunctionImport(mod, "set_array", "print", "set_array", BinaryenTypeCreate(std::data(types), std::size(types)), BinaryenTypeNone());
        }
        {
            BinaryenType locals[] = {
                size_type(),
                BinaryenTypeExternref(),
            };

            auto str     = local_get(0, type<value_types::string>()); // get string
            auto str_len = array_len(str);                            // get string len
            BinaryenAddFunction(mod,
                                "*lua_str_to_js_array",
                                type<value_types::string>(),
                                BinaryenTypeExternref(),
                                std::data(locals),
                                std::size(locals),
                                make_block(std::array{
                                    local_set(1, str_len),
                                    local_set(2, make_call("new_u8array", str_len, BinaryenTypeExternref())),
                                    make_if(local_get(1, size_type()),
                                            BinaryenLoop(mod,
                                                         "+loop",
                                                         make_block(std::array{
                                                             make_call("set_array",
                                                                       std::array{
                                                                           local_get(2, BinaryenTypeExternref()),
                                                                           local_tee(1, BinaryenBinary(mod, BinaryenSubInt32(), local_get(1, size_type()), const_i32(1)), size_type()),
                                                                           array_get(str, local_get(1, size_type()), char_type()),
                                                                       },
                                                                       BinaryenTypeNone()),
                                                             BinaryenBreak(mod,
                                                                           "+loop",
                                                                           local_get(1, size_type()),
                                                                           nullptr),
                                                         }))),

                                    BinaryenReturn(mod, local_get(2, BinaryenTypeExternref())),
                                }));
        }
        auto exp = make_call("*init", std::array{null(), null()}, ref_array_type());
        exp      = array_get(exp, const_i32(0), anyref());

        std::vector<std::tuple<const char*, value_types>> casts = {
            {
                "number",
                value_types::number,
            },
            {
                "integer",
                value_types::integer,
            },
            {
                "string",
                value_types::string,
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
            {
                func = "print_string";
                exp  = make_call("*lua_str_to_js_array", exp, BinaryenTypeExternref());
                break;
            }
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

        const char* tags[]              = {error_tag};
        BinaryenExpressionRef catches[] = {
            make_block(std::array{
                BinaryenDrop(mod, BinaryenPop(mod, anyref())),
                BinaryenUnreachable(mod),
            })};

        return BinaryenAddFunction(mod,
                                   "convert",
                                   BinaryenTypeNone(),
                                   BinaryenTypeNone(),
                                   std::data(locals),
                                   std::size(locals),
                                   BinaryenTry(mod,
                                               nullptr,
                                               make_block(switch_value(exp, casts, s)),
                                               std::data(tags),
                                               std::size(tags),
                                               std::data(catches),
                                               std::size(catches),
                                               nullptr));
    }
};
} // namespace wumbo