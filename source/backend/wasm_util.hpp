#pragma once

#include "binaryen-c.h"

#include <array>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

#include "nonstd/span.hpp"
#include "utils/util.hpp"

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

inline const char* type_name(value_type vtype)
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
        return BinaryenLocalGet(mod, static_cast<BinaryenIndex>(index), type);
    }

    expr_ref local_set(size_t index, expr_ref value)
    {
        return BinaryenLocalSet(mod, static_cast<BinaryenIndex>(index), value);
    }

    expr_ref local_tee(size_t index, expr_ref value, BinaryenType type)
    {
        return BinaryenLocalTee(mod, index, value, type);
    }

    expr_ref array_len(expr_ref array)
    {
        return BinaryenArrayLen(mod, array);
    }

    expr_ref array_set(expr_ref array, expr_ref index, expr_ref value)
    {
        return BinaryenArraySet(mod, array, index, value);
    }

    expr_ref array_get(expr_ref array, expr_ref index, BinaryenType type, bool is_signed = false)
    {
        return BinaryenArrayGet(mod, array, index, type, is_signed);
    }

    expr_ref make_block(nonstd::span<const expr_ref> list, const char* name = nullptr, BinaryenType btype = BinaryenTypeAuto())
    {
        if (list.size() == 1 && name == nullptr)
            return list[0];
        return BinaryenBlock(mod, name, const_cast<expr_ref*>(list.data()), list.size(), btype);
    }

    expr_ref make_call(const char* target, nonstd::span<const expr_ref> args, BinaryenType btype)
    {
        return BinaryenCall(mod, target, const_cast<expr_ref*>(args.data()), args.size(), btype);
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
    size_t data_name = 0;

    expr_ref add_string(const std::string& str)
    {
        auto name = std::to_string(data_name++);
        BinaryenAddDataSegment(mod, name.c_str(), "", true, 0, str.data(), str.size());
        return BinaryenArrayNewData(mod, BinaryenTypeGetHeapType(type<value_type::string>()), name.c_str(), const_i32(0), const_i32(str.size()));
    }

    void import_func(const char* name, BinaryenType params, BinaryenType results, const char* module_name, const char* import_name = nullptr)
    {
        BinaryenAddFunctionImport(mod, name, module_name, import_name ? import_name : name, params, results);
    }

    static constexpr size_t type_count      = 14;
    static constexpr size_t lua_type_offset = 5;

    std::array<BinaryenType, type_count> types;

    const char* error_tag = "error";

    template<value_type T>
    BinaryenType type() const
    {
        return types[static_cast<std::underlying_type_t<value_type>>(T) + lua_type_offset];
    }

    BinaryenType type(value_type t) const
    {
        return types[static_cast<std::underlying_type_t<value_type>>(t) + lua_type_offset];
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

    expr_ref unbox_number(expr_ref num)
    {
        return BinaryenStructGet(mod, 0, num, number_type(), false);
    }

    expr_ref unbox_integer(expr_ref num)
    {
        return BinaryenStructGet(mod, 0, num, integer_type(), false);
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

    template<typename F>
    auto switch_value(expr_ref exp, nonstd::span<const value_type> casts, F&& code)
    {
        auto n       = "nil" + std::to_string(label_counter++);
        auto counter = label_counter;
        exp          = BinaryenBrOn(mod, BinaryenBrOnNull(), n.c_str(), exp, BinaryenTypeNone());

        for (auto vtype : casts)
        {
            exp = BinaryenBrOn(mod, BinaryenBrOnCast(), (type_name(vtype) + std::to_string(label_counter++)).c_str(), exp, type(vtype));
        }

        expr_ref inner[] = {
            drop(exp),
            code(value_type{-1}, nullptr),
        };
        bool once = false;
        for (auto vtype : casts)
        {
            exp  = BinaryenBlock(mod, (type_name(vtype) + std::to_string(counter++)).c_str(), once ? &exp : std::data(inner), once ? 1 : std::size(inner), BinaryenTypeAuto());
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
    BinaryenType hash_entry_type() const
    {
        return types[4];
    }
    BinaryenType hash_array_type() const
    {
        return types[5];
    }

    template<typename Type, bool IsMutable = false, auto Pack = BinaryenPackedTypeNotPacked>
    struct member_desc
    {
        using type                        = Type;
        static constexpr const char* name = nullptr;
        static constexpr auto pack        = Pack;
        static constexpr bool is_mutable  = IsMutable;

        template<typename TypeBuilder, typename TypeArray>
        static BinaryenType get_type(TypeArray& build_types, TypeBuilderRef tb)
        {
            return TypeBuilder::template build_temp_type<type>(build_types, tb);
        }
    };

    template<typename... Member>
    struct struct_desc
    {
        using members = std::tuple<Member...>;

        template<typename TypeBuilder, typename TypeArray>
        static constexpr auto types(TypeArray& build_types, TypeBuilderRef tb)
        {
            return std::array{Member::template get_type<TypeBuilder>(build_types, tb)...};
        }
        static constexpr auto names()
        {
            return std::array{Member::name...};
        }
        static constexpr auto packs()
        {
            return std::array{Member::pack()...};
        }
        static constexpr auto mutables()
        {
            return std::array{Member::is_mutable...};
        }

        template<typename T>
        static expr_ref get(BinaryenModuleRef mod, expr_ref self, bool signed_ = false)
        {
            return BinaryenStructGet(mod, tuple_index_v<T, members>, self, T::type(), signed_);
        }

        template<typename T>
        static expr_ref set(BinaryenModuleRef mod, expr_ref self, expr_ref value)
        {
            return BinaryenStructSet(mod, tuple_index_v<T, members>, self, value);
        }
    };

    template<bool IsNullable = false>
    struct type_desc
    {
        static constexpr const char* name = nullptr;
        static constexpr bool nullable    = IsNullable;

        // template<typename type_builder>
        // static BinaryenType get_type()
        // {
        //     return BinaryenTypeFromHeapType(BinaryenTypeGetHeapType(type_builder<type_desc>::build(mod)[0]), nullable);
        // }
    };

    template<typename... Type>
    struct type_builder
    {
        static constexpr size_t type_count = sizeof...(Type);

        using types = std::tuple<Type...>;

        template<typename T>
        static BinaryenType build_temp_type(std::array<BinaryenType, type_count>& result, TypeBuilderRef tb)
        {
            if constexpr (std::is_base_of_v<type_desc<true>, T> || std::is_base_of_v<type_desc<false>, T>)
            {
                constexpr auto index = tuple_index_v<T, types>;
                if (result[index] == BinaryenType{})
                {
                    result[index] = BinaryenTypeFromHeapType(TypeBuilderGetTempHeapType(tb, index), T::nullable);
                }
                return result[index];
            }
            else
                return BinaryenTypeNone();
        }

        static auto build(ext_types& self)
        {
            TypeBuilderRef tb = TypeBuilderCreate(type_count);
            std::array<BinaryenType, type_count> result;

            BinaryenIndex i = 0;
            ([&]()
             {
                 auto types = Type::members::template types<type_builder<Type...>>(result, tb);
                 auto packs = Type::members::packs();
                 auto muts  = Type::members::mutables();
                 TypeBuilderSetStructType(
                     tb,
                     i++,
                     types.data(),
                     packs.data(),
                     muts.data(),
                     types.size());
             }(),
             ...);

            BinaryenHeapType heap_types[type_count];

            BinaryenIndex error_index;
            TypeBuilderErrorReason error_reason;
            if (!TypeBuilderBuildAndDispose(tb, (BinaryenHeapType*)&heap_types, &error_index, &error_reason))
            {
                throw;
            }
            i = 0;

            ([&]()
             {
                 result[i] = BinaryenTypeFromHeapType(heap_types[i], Type::nullable);
                 BinaryenModuleSetTypeName(self.mod, heap_types[i], Type::name);

                 auto names = Type::members::names();
                 for (size_t j = 0; j < std::size(names); ++j)
                     BinaryenModuleSetFieldName(self.mod, heap_types[i], j, names[j]);
             }(),
             ...);

            return result;
        }
    };

    struct int64
    {
        // BinaryenTypeInt64
    };

    struct float64
    {
        // BinaryenTypeFloat64
    };

    struct any
    {
        // BinaryenTypeAnyref
    };

    struct number : type_desc<>
    {
        static constexpr const char* name = "number";

        struct inner : member_desc<float64>
        {
        };

        using members = struct_desc<inner>;
    };

    struct integer : type_desc<>
    {
        static constexpr const char* name = "integer";

        struct inner : member_desc<int64>
        {
        };

        using members = struct_desc<inner>;
    };

    struct upvalue : type_desc<>
    {
        static constexpr const char* name = "upvalue";

        struct local_ref : member_desc<any, true>
        {
            static constexpr const char* name = "local_ref";
        };

        struct intre : member_desc<integer, true>
        {
            static constexpr const char* name = "integer";
        };

        using members = struct_desc<local_ref, intre>;
    };

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

        type_builder<number, integer, upvalue>::build(*this);

        TypeBuilderRef tb = TypeBuilderCreate(type_count);

        BinaryenType ref_array          = TypeBuilderGetTempRefType(tb, TypeBuilderGetTempHeapType(tb, 0), true);
        BinaryenType non_null_ref_array = TypeBuilderGetTempRefType(tb, TypeBuilderGetTempHeapType(tb, 0), false);

        BinaryenType lua_function  = TypeBuilderGetTempRefType(tb, TypeBuilderGetTempHeapType(tb, 3), true);
        BinaryenType upvalue       = TypeBuilderGetTempRefType(tb, TypeBuilderGetTempHeapType(tb, 1), true);
        BinaryenType upvalue_array = TypeBuilderGetTempRefType(tb, TypeBuilderGetTempHeapType(tb, 2), true);
        BinaryenType hash_entry    = TypeBuilderGetTempRefType(tb, TypeBuilderGetTempHeapType(tb, 4), true);
        BinaryenType hash_array    = TypeBuilderGetTempRefType(tb, TypeBuilderGetTempHeapType(tb, 5), true);
        BinaryenType table         = TypeBuilderGetTempRefType(tb, TypeBuilderGetTempHeapType(tb, 13), true);

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
                    {ref_array, ref_array},
                    {ref_array},
                },
            },
            {
                "hash_entry",
                struct_def{
                    {anyref(), anyref()},
                    {BinaryenPackedTypeNotPacked(), BinaryenPackedTypeNotPacked()},
                    {true, true},
                    {"key", "value"},
                },
            },
            {
                "hash_array",
                array_def{
                    hash_entry,
                    BinaryenPackedTypeNotPacked(),
                    true,
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
                    true,
                },
            },
            {
                "function",
                struct_def{
                    {lua_function, ref_array},
                    {BinaryenPackedTypeNotPacked(), BinaryenPackedTypeNotPacked()},
                    {false, true},
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
                    {ref_array, hash_array, table},
                    {BinaryenPackedTypeNotPacked(), BinaryenPackedTypeNotPacked(), BinaryenPackedTypeNotPacked()},
                    {true, true, true},
                    {"array", "hash", "metatable"},
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
        BinaryenAddTagExport(mod, error_tag, error_tag);

        types[static_cast<std::underlying_type_t<value_type>>(value_type::boolean) + lua_type_offset] = BinaryenTypeI31ref();
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
