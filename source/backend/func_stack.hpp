#pragma once

#include <algorithm>
#include <binaryen-c.h>
#include <cassert>
#include <optional>
#include <string>
#include <vector>

namespace wumbo
{

enum class var_type
{
    local,
    upvalue,
    global,
};

using local_index_t  = size_t;
using global_index_t = size_t;

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

    std::vector<std::string> label_stack;
    std::vector<std::string> request_label_stack;
    // count nested loops per function
    std::vector<std::string> loop_stack;

    size_t label_name = 0;

    void push_loop()
    {
        loop_stack.push_back(make_label("*loop"));
    }

    void pop_loop()
    {
        loop_stack.pop_back();
    }

    size_t unique_num()
    {
        return label_name++;
    }

    std::string make_label(std::string_view prefix)
    {
        return std::string{prefix} + std::to_string(unique_num());
    }
};

struct function_stack
{
    std::vector<size_t> blocks;
    std::vector<function_info> functions;
    std::vector<std::vector<size_t>> upvalues;

    std::vector<local_var> vars;

    void push_block()
    {
        blocks.push_back(vars.size());
    }

    void pop_block()
    {
        for (size_t i = blocks.back(); i < vars.size(); ++i)
        {
            auto& var = vars[i];
            if (!(var.flags & local_var::is_helper))
                var.flags &= ~local_var::is_used;
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
    }

    void pop_function()
    {
        auto func_offset = functions.back().offset;

        vars.resize(func_offset);
        functions.pop_back();
        upvalues.pop_back();
    }

    function_info& current_function()
    {
        return functions.back();
    }

    local_index_t local_offset(global_index_t index) const
    {
        assert(is_index_local(index) && "invalid index");
        auto& func = functions.back();
        return func.arg_count + (index - func.offset);
    }

    bool is_index_local(global_index_t index) const
    {
        auto func_offset = functions.back().offset;
        return index >= func_offset;
    }

    void free_local(size_t pos)
    {
        auto& func = functions.back();

        vars[(pos - func.arg_count) + func.offset].flags &= ~local_var::is_used;
    }

    local_index_t alloc_local(BinaryenType type, std::string_view name = "", bool helper = true)
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

    local_index_t alloc_lua_local(std::string_view name, BinaryenType type)
    {
        return alloc_local(type, name, false);
    }

    std::tuple<var_type, size_t, BinaryenType> find(const std::string& var_name) const
    {
        if (auto local = std::find_if(vars.rbegin(), vars.rend(), [&var_name](const local_var& var)
                                      {
                                          return !(var.flags & local_var::is_helper) && var.current_name() == var_name;
                                      });
            local != vars.rend())
        {
            auto pos = std::distance(vars.begin(), std::next(local).base());
            if (is_index_local(pos)) // local
                return {var_type::local, local_offset(pos), local->type};
            else // upvalue
                return {var_type::upvalue, pos, local->type};
        }
        return {var_type::global, 0, BinaryenTypeNone()}; // global
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
            {
                auto name = _self.vars[i].name + std::to_string(i);
                BinaryenFunctionSetLocalName(func, _self.local_offset(i), name.c_str());
            }
        }
    }

    std::vector<size_t> get_requested_upvalues()
    {
        return std::move(_self.upvalues.back());
    }
};

} // namespace wumbo
