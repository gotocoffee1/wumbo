#pragma once

#include <algorithm>
#include <cassert>
#include <optional>
#include <string>
#include <vector>

namespace wumbo::ast
{

enum class var_type
{
    local,
    upvalue,
    global,
};

struct local_var
{
    std::string name;
    local_usage* usage;
};

struct function_info
{
    size_t offset;
};

struct function_stack
{
    std::vector<size_t> blocks;
    std::vector<function_info> functions;
    std::vector<local_var> vars;

    void push_block()
    {
        blocks.push_back(vars.size());
    }

    void pop_block()
    {
        vars.resize(blocks.back());
        blocks.pop_back();
    }

    void push_function()
    {
        auto& func_info  = functions.emplace_back();
        func_info.offset = vars.size();
    }

    void pop_function()
    {
        auto func_offset = functions.back().offset;

        vars.resize(func_offset);
        functions.pop_back();
    }

    bool is_index_local(size_t index) const
    {
        if (functions.empty())
            return true;
        auto func_offset = functions.back().offset;
        return index >= func_offset;
    }

    void alloc_local(std::string_view name, local_usage& usage)
    {
        auto& var = vars.emplace_back();
        var.name  = name;
        var.usage = &usage;
    }

    std::tuple<var_type, local_usage*> find(const std::string& var_name) const
    {
        if (auto local = std::find_if(vars.rbegin(), vars.rend(), [&var_name](const local_var& var)
                                      {
                                          return var.name == var_name;
                                      });
            local != vars.rend())
        {
            auto pos = std::distance(vars.begin(), std::next(local).base());
            if (is_index_local(pos)) // local
                return {var_type::local, local->usage};
            else // upvalue
                return {var_type::upvalue, local->usage};
        }
        return {var_type::global, nullptr}; // global
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

    function_frame(function_stack& self)
        : _self{self}
    {
        _self.push_function();
    }

    ~function_frame()
    {
        _self.pop_function();
    }
};

} // namespace wumbo::ast
