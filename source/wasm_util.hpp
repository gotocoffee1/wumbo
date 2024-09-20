#pragma once

#include "binaryen-c.h"

namespace wumbo
{

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

struct utils
{
    BinaryenModuleRef mod;

    BinaryenExpressionRef const_i32(int32_t num)
    {
        return BinaryenConst(mod, BinaryenLiteralInt32(num));
    }

    BinaryenExpressionRef null()
    {
        return BinaryenRefNull(mod, BinaryenTypeNullref());
    }

    BinaryenExpressionRef local_get(size_t index, BinaryenType type)
    {
        return BinaryenLocalGet(mod, index, type);
    }

    BinaryenExpressionRef local_set(size_t index, BinaryenExpressionRef value)
    {
        return BinaryenLocalSet(mod, index, value);
    }

    BinaryenExpressionRef local_tee(size_t index, BinaryenExpressionRef value, BinaryenType type)
    {
        return BinaryenLocalTee(mod, index, value, type);
    }

    BinaryenExpressionRef array_len(BinaryenExpressionRef array)
    {
        return BinaryenArrayLen(mod, array);
    }

    BinaryenExpressionRef array_get(BinaryenExpressionRef array, BinaryenExpressionRef index, BinaryenType type, bool is_signed = false)
    {
        return BinaryenArrayGet(mod, array, index, type, is_signed);
    }

    template<size_t N>
    BinaryenExpressionRef make_block(std::array<BinaryenExpressionRef, N> list, const char* name = nullptr, BinaryenType btype = BinaryenTypeAuto())
    {
        static_assert(N > 1);
        return BinaryenBlock(mod, name, std::data(list), std::size(list), btype);
    } 
    
    BinaryenExpressionRef make_block(std::vector<BinaryenExpressionRef> list, const char* name = nullptr, BinaryenType btype = BinaryenTypeAuto())
    {
        if (list.size() == 1 && name == nullptr)
            return list[0];
        return BinaryenBlock(mod, name, std::data(list), std::size(list), btype);
    }

    static BinaryenType anyref()
    {
        return BinaryenTypeAnyref();
    }

    BinaryenExpressionRef resize_array(size_t new_array, BinaryenType type, BinaryenExpressionRef old_array, BinaryenExpressionRef grow, bool move_front = false)
    {
        return BinaryenArrayCopy(mod,
                                 BinaryenLocalTee(mod,
                                                  new_array,
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
    static constexpr size_t type_count = 12;

    BinaryenType types[type_count];

    template<value_types T>
    BinaryenType type() const
    {
        return types[static_cast<std::underlying_type_t<value_types>>(T) + 3];
    }

    BinaryenType type(value_types t) const
    {
        return types[static_cast<std::underlying_type_t<value_types>>(t) + 3];
    }

    BinaryenExpressionRef new_number(BinaryenExpressionRef num)
    {
        return BinaryenStructNew(mod, &num, 1, BinaryenTypeGetHeapType(type<value_types::number>()));
    }

    BinaryenExpressionRef new_integer(BinaryenExpressionRef num)
    {
        return BinaryenStructNew(mod, &num, 1, BinaryenTypeGetHeapType(type<value_types::integer>()));
    }

    static BinaryenType number_type()
    {
        return BinaryenTypeFloat64();
    }

    static BinaryenType integer_type()
    {
        return BinaryenTypeInt64();
    }

    static constexpr BinaryenIndex args_index     = 1;
    static constexpr BinaryenIndex upvalue_index  = 0;
    static constexpr BinaryenIndex func_arg_count = 2;

    static constexpr BinaryenIndex tbl_array_index = 0;
    static constexpr BinaryenIndex tbl_hash_index  = 1;

    std::size_t label_counter = 0;

    template<typename F>
    auto switch_value(BinaryenExpressionRef exp, const std::vector<std::tuple<const char*, value_types>>& casts, F&& code)
    {
        auto n = "nil_" + std::to_string(label_counter++);
        exp    = BinaryenBrOn(mod, BinaryenBrOnNull(), n.c_str(), exp, BinaryenTypeNone());

        for (auto& [name, vtype] : casts)
        {
            exp = BinaryenBrOn(mod, BinaryenBrOnCast(), name, exp, type(vtype));
        }

        BinaryenExpressionRef inner[] = {
            BinaryenDrop(mod, exp),
            code(value_types{-1}, nullptr),
        };
        bool once = false;
        for (auto& [name, vtype] : casts)
        {
            exp  = BinaryenBlock(mod, name, once ? &exp : std::data(inner), once ? 1 : std::size(inner), BinaryenTypeAuto());
            exp  = code(vtype, exp);
            once = true;
        }

        return std::array{
            BinaryenBlock(mod, n.c_str(), &exp, 1, BinaryenTypeAuto()),
            code(value_types::nil, nullptr),
        };
    }

    BinaryenExpressionRef new_value(BinaryenExpressionRef exp)
    {
        auto type = BinaryenExpressionGetType(exp);
        if (type == number_type())
            return new_number(exp);
        else if (type == integer_type())
            return new_integer(exp);
        else
        {
            return exp;
        }
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
                    BinaryenTypeInt32(),
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

        types[static_cast<std::underlying_type_t<value_types>>(value_types::boolean) + 3] = BinaryenTypeI31ref();
    }
};

} // namespace wumbo