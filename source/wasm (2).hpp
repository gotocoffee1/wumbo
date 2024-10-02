#pragma once

#include <vector>

template<typename T>
constexpr std::size_t size_uleb128(T val)
{
    static_assert(std::is_unsigned_v<T>);
    std::size_t count = 0;
    do
    {
        val >>= 7;
        ++count;
    } while (val != 0);
    return count;
}

template<typename T>
constexpr std::size_t write_uleb128(void* dest, T val)
{
    static_assert(std::is_unsigned_v<T>);
    std::byte* d      = static_cast<std::byte*>(dest);
    std::size_t count = 0;
    do
    {
        std::byte byte = static_cast<std::byte>(val) & std::byte{0x7f};
        val >>= 7;

        if (val != 0)
            byte |= std::byte{0x80}; // mark this byte to show that more bytes will follow

        d[count] = byte;
        ++count;
    } while (val != 0);

    return count;
}

template<typename T>
constexpr std::size_t write_leb128(void* dest, T val)
{
    static_assert(std::is_signed_v<T>);
    std::byte* d      = static_cast<std::byte*>(dest);
    std::size_t count = 0;
    bool cond;
    do
    {
        std::byte byte = static_cast<std::byte>(val) & std::byte{0x7f};
        val >>= 7;
        cond = !((val == 0 && (byte & std::byte{0x40}) == std::byte{0}) || (val == -1 && (byte & std::byte{0x40}) != std::byte{0}));
        if (cond)
            byte |= std::byte{0x80}; // mark this byte to show that more bytes will follow

        d[count] = byte;
        ++count;

    } while (cond);

    return count;
}

template<typename T>
constexpr std::size_t write_num(void* dest, T val)
{
    if constexpr (std::is_floating_point_v<T>)
    {
        std::memcpy(dest, &val, sizeof(val));
        return sizeof(T);
    }
    else if constexpr (std::is_signed_v<T>)
        return write_leb128(dest, val);
    else if constexpr (std::is_unsigned_v<T>)
        return write_uleb128(dest, val);
}

template<typename To, typename From>
To to(From val)
{
    return static_cast<To>(val);
}

namespace wasm
{

using byte = uint8_t;
using u32  = uint32_t;
using u64  = uint64_t;
using i32  = int32_t;
using i64  = int64_t;
using f64  = double;
using f32  = float;

using localidx  = u32;
using globalidx = u32;
using memidx    = u32;

template<typename T>
u32 csize(const T& c)
{
    return static_cast<u32>(std::size(c));
}

template<typename T>
u32 size_size(const T& c)
{
    return static_cast<u32>(size_uleb128(csize(c)));
}

enum class section_type : byte
{
    custom     = 0,
    type       = 1,
    import_    = 2,
    function   = 3,
    table      = 4,
    memory     = 5,
    global     = 6,
    export_    = 7,
    start      = 8,
    element    = 9,
    code       = 10,
    data       = 11,
    data_count = 12,
};

enum class val_types : byte
{
    // num types
    i32 = 0x7F,
    i64 = 0x7E,
    f32 = 0x7D,
    f64 = 0x7C,
    // ref types
    funcref   = 0x70,
    externref = 0x6F,
    // vec types
    v128 = 0x7B,
};

enum class mem
{
    i32_load = 0x28,
    i64_load = 0x29,
    f32_load = 0x2a,
    f64_load = 0x2b,

    i32_load8_s  = 0x2c,
    i32_load8_u  = 0x2d,
    i32_load16_s = 0x2e,
    i32_load16_u = 0x2f,

    i64_load8_s  = 0x30,
    i64_load8_u  = 0x31,
    i64_load16_s = 0x32,
    i64_load16_u = 0x33,
    i64_load32_s = 0x34,
    i64_load32_u = 0x35,

    i32_store = 0x36,
    i64_store = 0x37,
    f32_store = 0x38,
    f64_store = 0x39,

    i32_store8  = 0x3a,
    i32_store16 = 0x3b,

    i64_store8  = 0x3c,
    i64_store16 = 0x3d,
    i64_store32 = 0x3e,

    size = 0x3f,
    grow = 0x40,
};

enum class num
{
    i32_const = 0x41,
    i64_const = 0x42,
    f32_const = 0x43,
    f64_const = 0x44,

    i32_eqz  = 0x45,
    i32_eq   = 0x46,
    i32_ne   = 0x47,
    i32_lt_s = 0x48,
    i32_lt_u = 0x49,
    i32_gt_s = 0x4a,
    i32_gt_u = 0x4b,
    i32_le_s = 0x4c,
    i32_le_u = 0x4d,
    i32_ge_s = 0x4e,
    i32_ge_u = 0x4f,

    i64_eqz  = 0x50,
    i64_eq   = 0x51,
    i64_ne   = 0x52,
    i64_lt_s = 0x53,
    i64_lt_u = 0x54,
    i64_gt_s = 0x55,
    i64_gt_u = 0x56,
    i64_le_s = 0x57,
    i64_le_u = 0x58,
    i64_ge_s = 0x59,
    i64_ge_u = 0x5a,

    f32_eq = 0x5b,
    f32_ne = 0x5c,
    f32_lt = 0x5d,
    f32_gt = 0x5e,
    f32_le = 0x5f,
    f32_ge = 0x60,

    f64_eq = 0x61,
    f64_ne = 0x62,
    f64_lt = 0x63,
    f64_gt = 0x64,
    f64_le = 0x65,
    f64_ge = 0x66,

    i32_clz    = 0x67,
    i32_ctz    = 0x68,
    i32_popcnt = 0x69,
    i32_add    = 0x6a,
    i32_sub    = 0x6b,
    i32_mul    = 0x6c,
    i32_div_s  = 0x6d,
    i32_div_u  = 0x6e,
    i32_rem_s  = 0x6f,
    i32_rem_u  = 0x70,
    i32_and    = 0x71,
    i32_or     = 0x72,
    i32_xor    = 0x73,
    i32_shl    = 0x74,
    i32_shr_s  = 0x75,
    i32_shr_u  = 0x76,
    i32_rotl   = 0x77,
    i32_rotr   = 0x78,

    i64_clz    = 0x79,
    i64_ctz    = 0x7a,
    i64_popcnt = 0x7b,
    i64_add    = 0x7c,
    i64_sub    = 0x7d,
    i64_mul    = 0x7e,
    i64_div_s  = 0x7f,
    i64_div_u  = 0x80,
    i64_rem_s  = 0x81,
    i64_rem_u  = 0x82,
    i64_and    = 0x83,
    i64_or     = 0x84,
    i64_xor    = 0x85,
    i64_shl    = 0x86,
    i64_shr_s  = 0x87,
    i64_shr_u  = 0x88,
    i64_rotl   = 0x89,
    i64_rotr   = 0x8a,

    f32_abs      = 0x8b,
    f32_neg      = 0x8c,
    f32_ceil     = 0x8d,
    f32_floor    = 0x8e,
    f32_trunc    = 0x8f,
    f32_nearest  = 0x90,
    f32_sqrt     = 0x91,
    f32_add      = 0x92,
    f32_sub      = 0x93,
    f32_mul      = 0x94,
    f32_div      = 0x95,
    f32_min      = 0x96,
    f32_max      = 0x97,
    f32_copysign = 0x98,

    f64_abs      = 0x99,
    f64_neg      = 0x9a,
    f64_ceil     = 0x9b,
    f64_floor    = 0x9c,
    f64_trunc    = 0x9d,
    f64_nearest  = 0x9e,
    f64_sqrt     = 0x9f,
    f64_add      = 0xa0,
    f64_sub      = 0xa1,
    f64_mul      = 0xa2,
    f64_div      = 0xa3,
    f64_min      = 0xa4,
    f64_max      = 0xa5,
    f64_copysign = 0xa6,

    i32_wrap_i64    = 0xa7,
    i32_trunc_f32_s = 0xa8,
    i32_trunc_f32_u = 0xa9,
    i32_trunc_f64_s = 0xaa,
    i32_trunc_f64_u = 0xab,

    i64_extend_i32_s = 0xac,
    i64_extend_i32_u = 0xad,
    i64_trunc_f32_s  = 0xae,
    i64_trunc_f32_u  = 0xaf,
    i64_trunc_f64_s  = 0xb0,
    i64_trunc_f64_u  = 0xb1,

    f32_convert_i32_s = 0xb2,
    f32_convert_i32_u = 0xb3,
    f32_convert_i64_s = 0xb4,
    f32_convert_i64_u = 0xb5,
    f32_demote_f64    = 0xb6,

    f64_convert_i32_s = 0xb7,
    f64_convert_i32_u = 0xb8,
    f64_convert_i64_s = 0xb9,
    f64_convert_i64_u = 0xba,
    f64_promote_f32   = 0xbb,

    i32_reinterpret_f32 = 0xbc,
    i64_reinterpret_f64 = 0xbd,
    f32_reinterpret_i32 = 0xbe,
    f64_reinterpret_i64 = 0xbf,
};

enum class local
{
    get = 0x20,
    set = 0x21,
    tee = 0x22,
};

enum class global
{
    get = 0x23,
    set = 0x24,
};

enum class ctrl
{
    unreachable   = 0x00,
    nop           = 0x01,
    block         = 0x02,
    loop          = 0x03,
    if_           = 0x04,
    else_         = 0x05,
    end           = 0x0b,
    br            = 0x0c,
    br_if         = 0x0d,
    br_table      = 0x0e,
    return_       = 0x0f,
    call          = 0x10,
    call_indirect = 0x11,
};

enum class para
{
    drop   = 0x1a,
    select = 0x1b,
};

struct section
{
    section_type type;
    uint32_t size;
};

struct func_type
{
    std::vector<val_types> params;
    std::vector<val_types> results;

    u32 calc_func_type_size() const
    {
        return static_cast<u32>(1 + size_size(params) + csize(params) + size_size(results) + csize(results));
    }

    //friend bool operator==(const func_type&, const func_type&) = default;
};

struct func_body
{
    std::vector<std::pair<u32, val_types>> locals;
    std::vector<byte> instructions;

    u32 size() const
    {
        u32 size = 1 + size_size(locals) + csize(instructions);
        for (auto& [num, type] : locals)
            size += static_cast<u32>(size_uleb128(num) + 1);
        return size;
    }

    template<typename T>
    void add_num(T arg)
    {
        byte result[10];
        auto size = write_num(result, arg);
        for (size_t i = 0; i < size; ++i)
            instructions.push_back(result[i]);
    }

    template<typename Op, typename... T>
    void add_op(Op op, T... num)
    {
        static_assert(std::is_enum_v<Op>);
        instructions.push_back(static_cast<byte>(op));
        (add_num(num), ...);
    }
};

struct data
{
    u32 type;
    memidx mem = 0;
    std::vector<byte> instructions;
    std::vector<byte> data;
};

struct mod
{
    std::vector<func_type> types;
    std::vector<u32> functions;
    std::vector<func_body> code;
    std::vector<data> data;
};

struct function
{
    func_type sig;
    func_body body;
};

struct tmod
{
    std::vector<function> functions;
    std::vector<data> data;
};

void wat2wasm(const tmod& from, mod& to);

} // namespace wasm