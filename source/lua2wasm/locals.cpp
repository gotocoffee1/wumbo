#include "compiler.hpp"

namespace wumbo
{

std::vector<BinaryenExpressionRef> compiler::operator()(const local_function& p)
{
    size_t offset = local_offset();

    vars.emplace_back(p.name);
    bool is_upvalue = true;

    auto func = add_func_ref(p.name.c_str(), p.body);

    return {BinaryenLocalSet(mod,
                             offset + 0,
                             is_upvalue
                                 ? BinaryenStructNew(mod, &func, 1, BinaryenTypeGetHeapType(upvalue_type()))
                                 : func)};
}

std::vector<BinaryenExpressionRef> compiler::operator()(const local_variables& p)
{
    auto explist = (*this)(p.explist);
    auto local   = alloc_local(ref_array_type());

    auto res = unpack_locals(p.names, local_get(local, ref_array_type()));
    res.insert(res.begin(), local_set(local, explist));
    free_local(local);

    return res;
}

} // namespace wumbo
