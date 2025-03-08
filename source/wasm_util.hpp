#pragma once

#include "binaryen-c.h"

#include <array>
#include <vector>

#include "util.hpp"

namespace wumbo
{

template<typename... T>
auto create_type(T&&... types)
{
    std::array t{types...};
    return BinaryenTypeCreate(std::data(t), std::size(t));
}

static BinaryenType anyref()
{
    return BinaryenTypeAnyref();
}

enum class value_type
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
};

using expr_ref      = BinaryenExpressionRef;
using expr_ref_list = std::vector<expr_ref>;

template<typename T, typename U>
std::vector<T>& append(std::vector<T>& self, U&& other)
{
    self.insert(self.end(), std::begin(other), std::end(other));
    return self;
}

template<typename T, typename U>
std::vector<T> append(std::vector<T>&& self, U&& other)
{
    self.insert(self.end(), std::begin(other), std::end(other));
    return std::move(self);
}

struct utils
{
    BinaryenModuleRef mod;

    void export_func(const char* internal, const char* external = nullptr)
    {
        BinaryenAddFunctionExport(mod, internal, external ? external : internal);
    }

    expr_ref const_i32(int32_t num)
    {
        return BinaryenConst(mod, BinaryenLiteralInt32(num));
    }

    expr_ref null()
    {
        return BinaryenRefNull(mod, BinaryenTypeNullref());
    }

    expr_ref drop(expr_ref value)
    {
        return BinaryenDrop(mod, value);
    }

    expr_ref local_get(size_t index, BinaryenType type)
    {
        return BinaryenLocalGet(mod, index, type);
    }

    expr_ref local_set(size_t index, expr_ref value)
    {
        return BinaryenLocalSet(mod, index, value);
    }

    expr_ref local_tee(size_t index, expr_ref value, BinaryenType type)
    {
        return BinaryenLocalTee(mod, index, value, type);
    }

    expr_ref array_len(expr_ref array)
    {
        return BinaryenArrayLen(mod, array);
    }

    expr_ref array_get(expr_ref array, expr_ref index, BinaryenType type, bool is_signed = false)
    {
        return BinaryenArrayGet(mod, array, index, type, is_signed);
    }

    template<size_t N>
    expr_ref make_block(std::array<expr_ref, N> list, const char* name = nullptr, BinaryenType btype = BinaryenTypeAuto())
    {
        static_assert(N > 1);
        return BinaryenBlock(mod, name, std::data(list), std::size(list), btype);
    }

    expr_ref make_block(expr_ref_list list, const char* name = nullptr, BinaryenType btype = BinaryenTypeAuto())
    {
        if (list.size() == 1 && name == nullptr)
            return list[0];
        return BinaryenBlock(mod, name, std::data(list), std::size(list), btype);
    }

    template<size_t N>
    expr_ref make_call(const char* target, std::array<expr_ref, N> args, BinaryenType btype)
    {
        return BinaryenCall(mod, target, std::data(args), std::size(args), btype);
    }

    expr_ref make_call(const char* target, expr_ref arg, BinaryenType btype)
    {
        return BinaryenCall(mod, target, &arg, 1, btype);
    }

    expr_ref make_if(expr_ref cond, expr_ref if_true, expr_ref if_false = nullptr)
    {
        return BinaryenIf(mod, cond, if_true, if_false);
    }

    expr_ref make_return(expr_ref value = nullptr)
    {
        return BinaryenReturn(mod, value);
    }

    expr_ref resize_array(size_t new_array, BinaryenType type, expr_ref old_array, expr_ref grow, bool move_front = false)
    {
        return BinaryenArrayCopy(mod,
                                 local_tee(new_array,
                                           BinaryenArrayNew(mod,
                                                            BinaryenTypeGetHeapType(type),
                                                            BinaryenBinary(mod, BinaryenAddInt32(), grow, array_len(old_array)),
                                                            nullptr),
                                           type),
                                 move_front ? grow : const_i32(0),
                                 old_array,
                                 const_i32(0),
                                 array_len(old_array));
    }
};

struct ext_types : utils
{
    void import_func(const char* name, BinaryenType params, BinaryenType results, const char* module_name = "runtime")
    {
        BinaryenAddFunctionImport(mod, name, module_name, name, params, results);
    }

    static constexpr size_t type_count = 12;

    BinaryenType types[type_count];

    const char* error_tag = "error";

    template<value_type T>
    BinaryenType type() const
    {
        return types[static_cast<std::underlying_type_t<value_type>>(T) + 3];
    }

    BinaryenType type(value_type t) const
    {
        return types[static_cast<std::underlying_type_t<value_type>>(t) + 3];
    }

    bool big_int = true;
    bool big_num = true;

    expr_ref new_boolean(expr_ref num)
    {
        return BinaryenRefI31(mod, num);
    }

    expr_ref new_number(expr_ref num)
    {
        return BinaryenStructNew(mod, &num, 1, BinaryenTypeGetHeapType(type<value_type::number>()));
    }

    expr_ref new_integer(expr_ref num)
    {
        return BinaryenStructNew(mod, &num, 1, BinaryenTypeGetHeapType(type<value_type::integer>()));
    }

    static BinaryenType number_type()
    {
        return BinaryenTypeFloat64();
    }

    static BinaryenType integer_type()
    {
        return BinaryenTypeInt64();
    }

    expr_ref const_boolean(bool literal)
    {
        return const_i32(literal);
    }

    expr_ref const_integer(int64_t literal)
    {
        if (big_int)
            return BinaryenConst(mod, BinaryenLiteralInt64(literal));
        return BinaryenConst(mod, BinaryenLiteralInt32(literal));
    }

    expr_ref const_number(double literal)
    {
        if (big_num)
            return BinaryenConst(mod, BinaryenLiteralFloat64(literal));
        return BinaryenConst(mod, BinaryenLiteralFloat32(literal));
    }

    expr_ref binop(BinaryenOp op, expr_ref left, expr_ref right)
    {
        return BinaryenBinary(mod, op, left, right);
    }

    expr_ref unop(BinaryenOp op, expr_ref right)
    {
        return BinaryenUnary(mod, op, right);
    }

#define GEN_BINOP_INT(name, big, small)                       \
    expr_ref name(expr_ref left, expr_ref right)              \
    {                                                         \
        return binop(big_int ? big() : small(), left, right); \
    }

#define GEN_BINOP_NUM(name, big, small)                       \
    expr_ref name(expr_ref left, expr_ref right)              \
    {                                                         \
        return binop(big_num ? big() : small(), left, right); \
    }

#define GEN_UNNOP_INT(name, big, small)                \
    expr_ref name(expr_ref right)                      \
    {                                                  \
        return unop(big_int ? big() : small(), right); \
    }

#define GEN_UNOP_NUM(name, big, small)                 \
    expr_ref name(expr_ref right)                      \
    {                                                  \
        return unop(big_num ? big() : small(), right); \
    }

    GEN_BINOP_INT(mul_int, BinaryenMulInt64, BinaryenMulInt32)
    GEN_BINOP_INT(add_int, BinaryenAddInt64, BinaryenAddInt32)
    GEN_BINOP_INT(sub_int, BinaryenSubInt64, BinaryenSubInt32)
    GEN_BINOP_INT(div_int, BinaryenDivSInt64, BinaryenDivSInt32)
    GEN_BINOP_INT(rem_int, BinaryenRemSInt64, BinaryenRemSInt32)
    GEN_BINOP_INT(xor_int, BinaryenXorInt64, BinaryenXorInt32)
    GEN_BINOP_INT(or_int, BinaryenOrInt64, BinaryenOrInt32)
    GEN_BINOP_INT(and_int, BinaryenAndInt64, BinaryenAndInt32)
    GEN_BINOP_INT(shr_int, BinaryenShrSInt64, BinaryenShrSInt32)
    GEN_BINOP_INT(shl_int, BinaryenShlInt64, BinaryenShlInt32)
    GEN_BINOP_INT(eq_int, BinaryenEqInt64, BinaryenEqInt32)
    GEN_BINOP_INT(ne_int, BinaryenNeInt64, BinaryenNeInt32)
    GEN_BINOP_INT(gt_int, BinaryenGtSInt64, BinaryenGtSInt32)
    GEN_BINOP_INT(lt_int, BinaryenLtSInt64, BinaryenLtSInt32)
    GEN_BINOP_INT(le_int, BinaryenLeSInt64, BinaryenLeSInt32)
    GEN_BINOP_INT(ge_int, BinaryenGeSInt64, BinaryenGeSInt32)

    GEN_BINOP_INT(add_num, BinaryenAddFloat64, BinaryenAddFloat32)
    GEN_BINOP_INT(mul_num, BinaryenMulFloat64, BinaryenMulFloat32)
    GEN_BINOP_INT(sub_num, BinaryenSubFloat64, BinaryenSubFloat32)
    GEN_BINOP_INT(div_num, BinaryenDivFloat64, BinaryenDivFloat32)
    GEN_BINOP_INT(eq_num, BinaryenEqFloat64, BinaryenEqFloat32)
    GEN_BINOP_INT(ne_num, BinaryenNeFloat64, BinaryenNeFloat32)
    GEN_BINOP_INT(gt_num, BinaryenGtFloat64, BinaryenGtFloat32)
    GEN_BINOP_INT(lt_num, BinaryenLtFloat64, BinaryenLtFloat32)
    GEN_BINOP_INT(le_num, BinaryenLeFloat64, BinaryenLeFloat32)
    GEN_BINOP_INT(ge_num, BinaryenGeFloat64, BinaryenGeFloat32)

    GEN_UNOP_NUM(neg_num, BinaryenNegFloat64, BinaryenNegFloat32)

    expr_ref int_to_num(expr_ref right)
    {
        BinaryenOp op;
        if (big_num)
        {
            if (big_int)
                op = BinaryenConvertSInt64ToFloat64();
            else
                op = BinaryenConvertSInt32ToFloat64();
        }
        else
        {
            if (big_int)
                op = BinaryenConvertSInt64ToFloat32();
            else
                op = BinaryenConvertSInt32ToFloat32();
        }

        return unop(op, right);
    }

    expr_ref size_to_integer(expr_ref value)
    {
        if (big_int)
            return unop(BinaryenExtendUInt32(), value);
        return value;
    }

    static BinaryenType bool_type()
    {
        return BinaryenTypeInt32();
    }

    static BinaryenType size_type()
    {
        return BinaryenTypeInt32();
    }

    static BinaryenType char_type()
    {
        return BinaryenTypeInt32();
    }

    static constexpr BinaryenIndex args_index     = 1;
    static constexpr BinaryenIndex upvalue_index  = 0;
    static constexpr BinaryenIndex func_arg_count = 2;

    static constexpr BinaryenIndex tbl_array_index = 0;
    static constexpr BinaryenIndex tbl_hash_index  = 1;

    std::size_t label_counter = 0;

    static const char* to_string(value_type vtype)
    {
        switch (vtype)
        {
        case value_type::nil:
            return "nil";
        case value_type::boolean:
            return "boolean";
        case value_type::integer:
            return "integer";
        case value_type::number:
            return "number";
        case value_type::string:
            return "string";
        case value_type::function:
            return "function";
        case value_type::userdata:
            return "userdata";
        case value_type::thread:
            return "thread";
        case value_type::table:
            return "table";
        default:
            return "";
        }
    }

    template<typename F, size_t S>
    auto switch_value(expr_ref exp, const std::array<value_type, S>& casts, F&& code)
    {
        auto n       = "nil" + std::to_string(label_counter++);
        auto counter = label_counter;
        exp          = BinaryenBrOn(mod, BinaryenBrOnNull(), n.c_str(), exp, BinaryenTypeNone());

        for (auto vtype : casts)
        {
            exp = BinaryenBrOn(mod, BinaryenBrOnCast(), (to_string(vtype) + std::to_string(label_counter++)).c_str(), exp, type(vtype));
        }

        expr_ref inner[] = {
            drop(exp),
            code(value_type{-1}, nullptr),
        };
        bool once = false;
        for (auto vtype : casts)
        {
            exp  = BinaryenBlock(mod, (to_string(vtype) + std::to_string(counter++)).c_str(), once ? &exp : std::data(inner), once ? 1 : std::size(inner), BinaryenTypeAuto());
            exp  = code(vtype, exp);
            once = true;
        }

        return std::array{
            BinaryenBlock(mod, n.c_str(), &exp, 1, BinaryenTypeAuto()),
            code(value_type::nil, nullptr),
        };
    }

    BinaryenType ref_array_type() const
    {
        return types[0];
    }

    BinaryenType lua_func() const
    {
        return types[3];
    }

    BinaryenType upvalue_type() const
    {
        return types[1];
    }

    BinaryenType upvalue_array_type() const
    {
        return types[2];
    }

    void build_types()
    {
        BinaryenModuleSetFeatures(
            mod,
            BinaryenModuleGetFeatures(mod)
                | BinaryenFeatureTailCall()
                | BinaryenFeatureGC()
                | BinaryenFeatureExceptionHandling()
                | BinaryenFeatureBulkMemory()
                | BinaryenFeatureReferenceTypes());

        struct struct_def
        {
            std::vector<BinaryenType> field_types;
            std::vector<BinaryenPackedType> field_packs;
            std::vector<uint8_t> field_muts; // classic c++
            std::vector<const char*> field_names;
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

        TypeBuilderRef tb = TypeBuilderCreate(type_count);

        BinaryenType ref_array          = TypeBuilderGetTempRefType(tb, TypeBuilderGetTempHeapType(tb, 0), true);
        BinaryenType non_null_ref_array = TypeBuilderGetTempRefType(tb, TypeBuilderGetTempHeapType(tb, 0), false);

        BinaryenType lua_function  = TypeBuilderGetTempRefType(tb, TypeBuilderGetTempHeapType(tb, 3), true);
        BinaryenType upvalue       = TypeBuilderGetTempRefType(tb, TypeBuilderGetTempHeapType(tb, 1), true);
        BinaryenType upvalue_array = TypeBuilderGetTempRefType(tb, TypeBuilderGetTempHeapType(tb, 2), true);

        def defs[] = {
            {
                "ref_array",
                array_def{
                    BinaryenTypeAnyref(),
                    BinaryenPackedTypeNotPacked(),
                    true,
                },
            },
            {
                "upvalue",
                struct_def{
                    {anyref()},
                    {BinaryenPackedTypeNotPacked()},
                    {true},
                    {"local_ref"},
                },
                false,
            },
            {
                "upvalue_array",
                array_def{
                    upvalue,
                    BinaryenPackedTypeNotPacked(),
                    true,
                },
            },
            {
                "lua_function",
                sig_def{
                    {upvalue_array, ref_array},
                    {ref_array},
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
                    {integer_type()},
                    {BinaryenPackedTypeNotPacked()},
                    {false},
                },
            },
            {
                "number",
                struct_def{
                    {number_type()},
                    {BinaryenPackedTypeNotPacked()},
                    {false},
                },
            },
            {
                "string",
                array_def{
                    char_type(),
                    BinaryenPackedTypeInt8(),
                    false,
                },
            },
            {
                "function",
                struct_def{
                    {lua_function, upvalue_array},
                    {BinaryenPackedTypeNotPacked(), BinaryenPackedTypeNotPacked()},
                    {false, false},
                    {"function_ref", "upvalues"},
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
                    {ref_array, ref_array},
                    {BinaryenPackedTypeNotPacked(), BinaryenPackedTypeNotPacked()},
                    {true, true},
                    {"array", "hash"},
                },
            },
        };

        static_assert(type_count == std::size(defs));

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
                               TypeBuilderSetSignatureType(
                                   tb,
                                   i,
                                   TypeBuilderGetTempTupleType(tb, params.data(), params.size()),
                                   TypeBuilderGetTempTupleType(tb, returns.data(), returns.size()));

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

            std::visit(overload{
                           [&](array_def& def) {

                           },
                           [&](struct_def& def)
                           {
                               for (size_t j = 0; j < std::size(def.field_names); ++j)
                                   BinaryenModuleSetFieldName(mod, heap_types[i], j, def.field_names[j]);
                           },
                           [&](sig_def& def) {

                           },
                       },
                       defs[i].inner);
        }

        BinaryenAddTag(mod, error_tag, anyref(), BinaryenTypeNone());

        types[static_cast<std::underlying_type_t<value_type>>(value_type::boolean) + 3] = BinaryenTypeI31ref();
    }

    expr_ref throw_error(expr_ref error)
    {
        return BinaryenThrow(mod, error_tag, &error, 1);
    }

    [[noreturn]] void semantic_error(std::string msg)
    {
        throw std::runtime_error(std::move(msg));
    }
};

} // namespace wumbo
