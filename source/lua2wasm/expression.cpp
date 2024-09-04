#include "compiler.hpp"

namespace wumbo
{
BinaryenExpressionRef compiler::operator()(const expression_list& p)
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

                std::vector<BinaryenExpressionRef> copy;

                copy.push_back(BinaryenArrayCopy(mod,
                                                 BinaryenLocalTee(mod,
                                                                  new_array,
                                                                  BinaryenArrayNew(mod,
                                                                                   BinaryenTypeGetHeapType(type),
                                                                                   BinaryenBinary(mod, BinaryenAddInt32(), const_i32(result.size()), BinaryenArrayLen(mod, l_get)),
                                                                                   nullptr),
                                                                  type),
                                                 const_i32(result.size()),
                                                 l_get,
                                                 const_i32(0),
                                                 BinaryenArrayLen(mod, l_get)));
                size_t j = 0;
                for (auto& init : result)
                    copy.push_back(BinaryenArraySet(mod, local_get(new_array, type), const_i32(j++), init));

                copy.push_back(local_get(new_array, type));

                result.push_back(null());
                free_local(local);
                free_local(new_array);
                return BinaryenIf(mod,
                                  BinaryenRefIsNull(mod, exp),
                                  BinaryenArrayNewFixed(mod, BinaryenTypeGetHeapType(ref_array_type()), std::data(result), std::size(result)),
                                  BinaryenBlock(mod, "", std::data(copy), std::size(copy), type));
            }
            else
            {
                exp = BinaryenIf(mod,
                                 BinaryenRefIsNull(mod, exp),
                                 null(),
                                 BinaryenArrayGet(mod, l_get, const_i32(0), anyref(), false));

                result.push_back(exp);
                free_local(local);
            }
        }
        else
            result.push_back(new_value(exp));
    }

    return BinaryenArrayNewFixed(mod, BinaryenTypeGetHeapType(ref_array_type()), std::data(result), std::size(result));
}

BinaryenExpressionRef compiler::operator()(const expression& p)
{
    return std::visit(*this, p.inner);
}
} // namespace wumbo