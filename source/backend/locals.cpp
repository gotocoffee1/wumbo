#include "backend/wasm_util.hpp"
#include "binaryen-c.h"
#include "compiler.hpp"

namespace wumbo
{

expr_ref_list compiler::operator()(const local_function& p)
{
    bool is_upvalue = p.usage.is_upvalue();

    auto index = _func_stack.alloc_lua_local(p.name, is_upvalue ? upvalue_type() : anyref());
    if (is_upvalue)
    {
        auto func = add_func_ref(p.name.c_str(), p.body);
        return {
            BinaryenStructSet(mod, 0, local_tee(index, BinaryenStructNew(mod, nullptr, 0, BinaryenTypeGetHeapType(upvalue_type())), upvalue_type()), func),
        };
    }
    else
    {
        auto [ref, ups] = get_func_ref(p.name.c_str(), p.body.params, p.body.usage, p.body.vararg, [&]()
                                       {
                                           return (*this)(p.body.inner);
                                       });

        expr_ref exp[] = {
            ref,
            null(),
        };
        auto func = BinaryenStructNew(mod, std::data(exp), std::size(exp), BinaryenTypeGetHeapType(type<value_type::function>()));

        if (ups.empty())
        {
            return {
                local_set(index, func),
            };
        }
        return {
            BinaryenStructSet(mod, 1, BinaryenRefCast(mod, local_tee(index, func, anyref()), type<value_type::function>()), BinaryenArrayNewFixed(mod, BinaryenTypeGetHeapType(ref_array_type()), std::data(ups), std::size(ups))),
        };
    }
}

expr_ref_list compiler::operator()(const local_variables& p)
{
    auto explist = (*this)(p.explist);
    auto local   = help_var_scope{_func_stack, ref_array_type()};

    auto res = unpack_locals(p.names, local_get(local, ref_array_type()), p.usage);
    res.insert(res.begin(), local_set(local, explist));

    return res;
}

} // namespace wumbo
