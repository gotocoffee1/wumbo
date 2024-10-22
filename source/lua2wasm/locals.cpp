#include "compiler.hpp"

namespace wumbo
{

expr_ref_list compiler::operator()(const local_function& p)
{
    auto index      = _func_stack.alloc_lua_local(p.name, upvalue_type());
    bool is_upvalue = true;

    auto func = add_func_ref(p.name.c_str(), p.body);

    if (is_upvalue)
        return {BinaryenStructSet(mod, 0, local_tee(index, BinaryenStructNew(mod, nullptr, 0, BinaryenTypeGetHeapType(upvalue_type())), upvalue_type()), func)};

    return {local_set(index, func)};
}

expr_ref_list compiler::operator()(const local_variables& p)
{
    auto explist = (*this)(p.explist);
    auto local   = help_var_scope{_func_stack, ref_array_type()};

    auto res = unpack_locals(p.names, local_get(local, ref_array_type()));
    res.insert(res.begin(), local_set(local, explist));

    return res;
}

} // namespace wumbo
