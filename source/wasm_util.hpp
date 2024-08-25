#pragma once

#include "binaryen-c.h"

namespace lua2wasm
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

    template<size_t N>
    BinaryenExpressionRef make_block(std::array<BinaryenExpressionRef, N> list, const char* name = nullptr, BinaryenType btype = BinaryenTypeAuto())
    {
        return BinaryenBlock(mod, name, std::data(list), std::size(list), btype);
    }

    static BinaryenType anyref()
    {
        return BinaryenTypeAnyref();
    }
};

struct ext_types : utils
{
    BinaryenType types[11];

    template<value_types T>
    BinaryenType type() const
    {
        return types[static_cast<std::underlying_type_t<value_types>>(T) + 2];
    }

    BinaryenType type(value_types t) const
    {
        return types[static_cast<std::underlying_type_t<value_types>>(t) + 2];
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
        return types[1];
    }

    BinaryenType pair_list() const
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

        BinaryenType pair[] = {
            BinaryenTypeAnyref(),
            BinaryenTypeAnyref(),
        };

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
                "lua_function",
                sig_def{
                    {},
                    {},
                },
            },
            {
                "pair_list",
                array_def{
                    BinaryenTypeCreate(std::data(pair), std::size(pair)),
                    BinaryenPackedTypeNotPacked(),
                    false,
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
                    {BinaryenTypeNone(), BinaryenTypeNone()},
                    {BinaryenPackedTypeNotPacked(), BinaryenPackedTypeNotPacked()},
                    {false, false},
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
                    {BinaryenTypeNone(), BinaryenTypeNone()},
                    {BinaryenPackedTypeNotPacked(), BinaryenPackedTypeNotPacked()},
                    {false, false},
                },
            },
        };

        TypeBuilderRef tb = TypeBuilderCreate(std::size(defs));

        BinaryenType ref_array          = TypeBuilderGetTempRefType(tb, TypeBuilderGetTempHeapType(tb, 0), true);
        BinaryenType non_null_ref_array = TypeBuilderGetTempRefType(tb, TypeBuilderGetTempHeapType(tb, 0), false);

        std::get<sig_def>(defs[1].inner).param_types.assign({ref_array, ref_array});
        std::get<sig_def>(defs[1].inner).return_types.assign({ref_array});

        BinaryenType lua_function = TypeBuilderGetTempRefType(tb, TypeBuilderGetTempHeapType(tb, 1), true);

        std::get<struct_def>(defs[7].inner).field_types[0] = lua_function;
        std::get<struct_def>(defs[7].inner).field_types[1] = ref_array;

        std::get<struct_def>(defs[10].inner).field_types[tbl_array_index] = TypeBuilderGetTempRefType(tb, TypeBuilderGetTempHeapType(tb, 0), true);
        std::get<struct_def>(defs[10].inner).field_types[tbl_hash_index]  = TypeBuilderGetTempRefType(tb, TypeBuilderGetTempHeapType(tb, 0), true);

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
                               TypeBuilderSetSignatureType(tb, i, TypeBuilderGetTempTupleType(tb, params.data(), params.size()), TypeBuilderGetTempTupleType(tb, returns.data(), returns.size()));

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
        }
    }
};

} // namespace lua2wasm