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
        static constexpr flag_t is_helper = 1 << 1;
    };

    struct function_info
    {
        size_t offset;
        size_t arg_count;
        std::optional<size_t> vararg_offset;
    };

    struct function_stack
    {
        // count nested loops per function
        std::vector<size_t> loop_stack;
        size_t loop_counter;

        std::vector<size_t> blocks;
        std::vector<function_info> functions;
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

        void push_function(size_t func_arg_count, std::optional<size_t> vararg_offset)
        {
            upvalues.emplace_back();
            auto& func_info         = functions.emplace_back();
            func_info.offset        = vars.size();
            func_info.arg_count     = func_arg_count;
            func_info.vararg_offset = vararg_offset;

            loop_stack.push_back(0);
        }

        void pop_function()
        {
            auto func_offset = functions.back().offset;

            loop_stack.pop_back();

            vars.resize(func_offset);
            functions.pop_back();
            upvalues.pop_back();
        }

        size_t local_offset(size_t index) const
        {
            auto& func = functions.back();
            return func.arg_count + (index - func.offset);
        }

        size_t local_offset() const
        {
            return local_offset(vars.size());
        }

        bool is_index_local(size_t index) const
        {
            auto func_offset = functions.back().offset;
            return index >= func_offset;
        }

        void free_local(size_t pos)
        {
            auto& func = functions.back();

            vars[(pos - func.arg_count) + func.offset].flags &= ~local_var::is_used;
        }

        size_t alloc_local(BinaryenType type, std::string_view name = "", bool helper = true)
        {
            auto func_offset = functions.back().offset;

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

        function_frame(function_stack& self, size_t func_arg_count, std::optional<size_t> vararg_offset = std::nullopt)
            : _self{self}
        {
            _self.push_function(func_arg_count, vararg_offset);
        }

        ~function_frame()
        {
            _self.pop_function();
        }

        std::vector<BinaryenType> get_local_type_list() const
        {
            auto func_offset = _self.functions.back().offset;

            std::vector<BinaryenType> locals;
            for (size_t i = func_offset; i < _self.vars.size(); ++i)
                locals.push_back(_self.vars[i].type);
            return locals;
        }

        void set_local_names(BinaryenFunctionRef func) const
        {
            auto func_offset = _self.functions.back().offset;

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

    expr_ref get_upvalue(size_t index)
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

    expr_ref get_var(const name_t& name)
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

    expr_ref set_var(const name_t& name, expr_ref value)
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

    expr_ref _funchead(const funchead& p);
    expr_ref _functail(const functail& p, expr_ref function);

    expr_ref _vartail(const vartail& p, expr_ref var);
    expr_ref _vartail_set(const vartail& p, expr_ref var, expr_ref value);

    expr_ref _varhead(const varhead& v);

    expr_ref _varhead_set(const varhead& v, expr_ref value);
    expr_ref at_or_null(size_t array, size_t index, expr_ref value = nullptr)
    {
        return BinaryenIf(mod,
                          BinaryenRefIsNull(mod, value ? local_tee(array, value, ref_array_type()) : local_get(array, ref_array_type())),
                          null(),
                          BinaryenIf(mod,
                                     BinaryenBinary(mod, BinaryenGtUInt32(), BinaryenArrayLen(mod, local_get(array, ref_array_type())), const_i32(index)),
                                     BinaryenArrayGet(mod, local_get(array, ref_array_type()), const_i32(index), anyref(), false),
                                     null()));
    }

    expr_ref_list operator()(const function_call& p);

    expr_ref_list operator()(const assignments& p);

    auto operator()(const label_statement& p)
    {
        expr_ref_list result;
        return result;
    }
    auto operator()(const goto_statement& p)
    {
        expr_ref_list result;

        //BinaryenBreak(mod, p.name.c_str(), nullptr, nullptr);

        return result;
    }
    auto to_bool()
    {
        auto casts = std::array{
            value_types::boolean,
        };

        BinaryenAddFunction(mod,
                            "*to_bool",
                            anyref(),
                            bool_type(),
                            nullptr,
                            0,
                            make_block(switch_value(local_get(0, anyref()), casts, [&](value_types type, expr_ref exp)
                                                    {
                                                        switch (type)
                                                        {
                                                        case value_types::nil:
                                                            return make_return(const_i32(0));
                                                        case value_types::boolean:
                                                            return make_return(BinaryenI31Get(mod, exp, false));
                                                        default:
                                                            return make_return(const_i32(1));
                                                        }
                                                    })));
        BinaryenAddFunction(mod,
                            "*to_bool_invert",
                            anyref(),
                            bool_type(),
                            nullptr,
                            0,
                            make_block(switch_value(local_get(0, anyref()), casts, [&](value_types type, expr_ref exp)
                                                    {
                                                        switch (type)
                                                        {
                                                        case value_types::nil:
                                                            return make_return(const_i32(1));
                                                        case value_types::boolean:
                                                            return make_return(BinaryenUnary(mod, BinaryenEqZInt32(), BinaryenI31Get(mod, exp, false)));
                                                        default:
                                                            return make_return(const_i32(0));
                                                        }
                                                    })));
    }
    expr_ref_list operator()(const do_statement& p)
    {
        return (*this)(p.inner);
    }

    expr_ref_list operator()(const if_statement& p)
    {
        expr_ref last  = nullptr;
        expr_ref first = nullptr;
        for (auto& [cond_exp, body] : p.cond_block)
        {
            auto cond = (*this)(cond_exp);
            auto next = make_if(make_call("*to_bool", cond, bool_type()), make_block((*this)(body)));
            if (last)
                BinaryenIfSetIfFalse(last, next);
            else
                first = next;
            last = next;
        }
        if (p.else_block)
            BinaryenIfSetIfFalse(last, make_block((*this)(*p.else_block)));

        return {first};
    }

    expr_ref_list operator()(const for_statement& p);
    expr_ref_list operator()(const key_break& p);
    expr_ref_list operator()(const while_statement& p);
    expr_ref_list operator()(const repeat_statement& p);
    expr_ref_list operator()(const for_each& p);

    expr_ref_list operator()(const function_definition& p);
    expr_ref_list operator()(const local_function& p);
    expr_ref_list operator()(const local_variables& p);

    expr_ref operator()(const expression& p);

    expr_ref operator()(const expr_ref_list& p);
    expr_ref operator()(const expression_list& p);

    auto operator()(const nil&)
    {
        return null();
    }

    auto operator()(const boolean& p)
    {
        return new_boolean(const_boolean(p.value));
    }

    auto operator()(const int_type& p)
    {
        return new_integer(const_integer(p));
    }

    auto operator()(const float_type& p)
    {
        return new_number(const_number(p));
    }

    expr_ref add_string(const std::string& str)
    {
        auto name = std::to_string(data_name++);
        BinaryenAddDataSegment(mod, name.c_str(), "", true, 0, str.data(), str.size());
        return BinaryenArrayNewData(mod, BinaryenTypeGetHeapType(type<value_types::string>()), name.c_str(), const_i32(0), const_i32(str.size()));
    }

    expr_ref operator()(const literal& p)
    {
        return add_string(p.str);
    }

    expr_ref operator()(const ellipsis& p)
    {
        auto vararg_offset = _func_stack.functions.back().vararg_offset;
        if (vararg_offset)
        {
            return local_get(*vararg_offset, ref_array_type());
        }
        semantic_error("cannot use '...' outside a vararg function near '...'");
    }

    expr_ref_list unpack_locals(const name_list& p, expr_ref list, bool is_vararg = false)
    {
        auto offset                    = _func_stack.local_offset();
        std::string none               = "+none" + std::to_string(label_name++);
        std::vector<const char*> names = {none.c_str()};
        bool is_upvalue                = true;
        expr_ref_list result;

        if (p.empty())
        {
            if (is_vararg)
                _func_stack.functions.back().vararg_offset = args_index;
            return result;
        }

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

        const char* vararg = "...";
        if (is_vararg)
        {
            names.push_back(vararg);
        }

        std::array<expr_ref, 2> exp = {
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
        if (is_vararg)
        {
            exp[0] = BinaryenBlock(mod, vararg, std::data(exp), 1, BinaryenTypeAuto());

            auto new_array                             = _func_stack.alloc_lua_local(vararg, ref_array_type());
            _func_stack.functions.back().vararg_offset = new_array;

            auto size = help_var_scope{_func_stack, size_type()};

            auto calc_size = local_tee(size, BinaryenBinary(mod, BinaryenSubInt32(), array_len(list), const_i32(p.size())), size_type());
            exp[1]         = BinaryenArrayCopy(mod,
                                       local_tee(new_array,
                                                 BinaryenArrayNew(mod,
                                                                  BinaryenTypeGetHeapType(ref_array_type()),
                                                                  calc_size,
                                                                  nullptr),
                                                 ref_array_type()),
                                       const_i32(0),
                                       list,
                                       const_i32(p.size()),
                                       local_get(size, size_type()));
        }

        bool first = !is_vararg;
        size_t j   = p.size();
        for (auto iter = p.rbegin(); iter != p.rend(); ++iter)
        {
            j--;
            exp[0] = BinaryenBlock(mod, iter->c_str(), std::data(exp), first ? 1 : std::size(exp), BinaryenTypeAuto());
            first  = false;

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

        expr_ref_list body = unpack_locals(p, local_get(args_index, ref_array_type()), vararg);

        append(body, f());

        body.push_back(make_return(null()));

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

        return std::tuple{result, frame.get_requested_upvalues()};
    }

    expr_ref_list gather_upvalues(const std::vector<size_t>& req_ups)
    {
        expr_ref_list ups;

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
        return ups;
    }

    template<typename F>
    auto add_func_ref(const char* name, const name_list& p, bool vararg, F&& f)
    {
        auto [func, req_ups] = add_func(name, p, vararg, f);

        auto ups = gather_upvalues(req_ups);

        auto sig       = BinaryenTypeFromHeapType(BinaryenFunctionGetType(func), false);
        expr_ref exp[] = {
            BinaryenRefFunc(mod, name, sig),
            ups.empty() ? null() : BinaryenArrayNewFixed(mod, BinaryenTypeGetHeapType(upvalue_array_type()), std::data(ups), std::size(ups)),
        };
        return BinaryenStructNew(mod, std::data(exp), std::size(exp), BinaryenTypeGetHeapType(type<value_types::function>()));
    }

    auto add_func_ref(const char* name, const block& inner, const name_list& p, bool vararg)
    {
        return add_func_ref(
            name, p, vararg, [&]()
            {
                return (*this)(inner);
            });
    }

    auto add_func_ref(const char* name, const function_body& p) -> expr_ref
    {
        return add_func_ref(name, p.inner, p.params, p.vararg);
    }

    auto operator()(const function_body& p)
    {
        return add_func_ref(std::to_string(function_name++).c_str(), p);
    }

    expr_ref operator()(const prefixexp& p);

    template<typename T>
    auto operator()(const box<T>& p)
    {
        return (*this)(*p);
    }

    expr_ref calc_hash(expr_ref key);
    expr_ref find_bucket(expr_ref table, expr_ref hash);

    expr_ref table_get(expr_ref table, expr_ref key);

    expr_ref table_set(expr_ref table, expr_ref key, expr_ref value);

    BinaryenFunctionRef compare(const char* name, value_types vtype);
    void func_table_get();
    expr_ref operator()(const table_constructor& p);

    expr_ref call(expr_ref func, expr_ref args);

    expr_ref_list open_basic_lib();
    expr_ref_list setup_env();

    void make_bin_operation();

    expr_ref operator()(const bin_operation& p);

    void make_un_operation();

    expr_ref operator()(const un_operation& p);

    auto operator()(const block& p) -> expr_ref_list
    {
        block_scope block{_func_stack};

        expr_ref_list result;
        for (auto& statement : p.statements)
        {
            auto temp = std::visit(*this, statement.inner);
            result.insert(result.end(), temp.begin(), temp.end());
        }
        if (p.retstat)
        {
            auto list = (*this)(*p.retstat);
            result.push_back(make_return(list));
        }

        return result;
    }

    auto convert(const block& chunk)
    {
        func_table_get();
        to_bool();
        make_un_operation();
        make_bin_operation();

        function_frame frame{_func_stack, 0, std::nullopt};

        auto env = setup_env();

        append(env, open_basic_lib());

        auto start = add_func_ref("*init", chunk, {}, true);

        env.push_back(BinaryenDrop(mod, call(start, null())));

        const char* tags[] = {error_tag};
        expr_ref catches[] = {
            make_block(std::array{
                BinaryenDrop(mod, BinaryenPop(mod, anyref())),
                BinaryenNop(mod),
                //BinaryenUnreachable(mod),
            })};

        auto try_ = BinaryenTry(mod,
                                nullptr,
                                make_block(env),
                                std::data(tags),
                                std::size(tags),
                                std::data(catches),
                                std::size(catches),
                                nullptr);

        auto locals = frame.get_local_type_list();
        BinaryenAddFunction(mod,
                            "*invoke_lua",
                            BinaryenTypeNone(),
                            BinaryenTypeNone(),
                            std::data(locals),
                            std::size(locals),
                            try_);

        BinaryenAddFunctionExport(mod, "*invoke_lua", "start");
    }
};
} // namespace wumbo