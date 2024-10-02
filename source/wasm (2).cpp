#include "wasm.hpp"

#include <ostream>

namespace wasm
{

using namespace std::string_view_literals;

void wat2wasm(const tmod& from, mod& to)
{
    for (auto& f : from.functions)
    {
        u32 index = 0;
        for (auto& t : to.types)
        {
            // TODO
            //if (t == f.sig)
            {
                to.functions.push_back(index);
                break;
            }
            index++;
        }
        if (index == to.types.size())
        {
            to.functions.push_back(index);
            to.types.emplace_back(f.sig);
        }

        to.code.emplace_back(f.body);
    }
    to.data = from.data;
}

struct writer
{
    std::ostream& f;
    char buffer[10];

    template<typename T>
    void write(T val)
    {
        if constexpr (std::is_integral_v<T> || std::is_enum_v<T>)
            f.write(reinterpret_cast<const char*>(&val), sizeof(T));
        else
            f << val;
    }

    template<typename T>
    void write_compressed(T val)
    {
        f << std::string_view(buffer, write_uleb128(buffer, val));
    }
};
} // namespace wasm

void to_stream(std::ostream& f, const wasm::mod& m)
{
    using namespace wasm;
    writer w{f};
    w.write("\0asm"sv);            // magic
    w.write("\x01\x00\x00\x00"sv); // version

    if (!m.types.empty())
    {
        w.write(section_type::type);
        auto section_size = size_size(m.types);
        for (auto& func : m.types)
        {
            section_size += func.calc_func_type_size();
        }
        w.write_compressed(section_size);
        w.write_compressed(csize(m.types));
        for (auto& func : m.types)
        {
            w.write("\x60"sv);
            w.write_compressed(csize(func.params));
            for (auto& p : func.params)
                w.write(p);

            w.write_compressed(csize(func.results));
            for (auto& r : func.results)
                w.write(r);
        }
    }
    if (!m.functions.empty())
    {
        w.write(section_type::function);

        auto section_size = size_size(m.functions);

        for (auto& func : m.functions)
        {
            section_size += static_cast<u32>(size_uleb128(func));
        }
        w.write_compressed(section_size);
        w.write_compressed(csize(m.functions));

        for (auto& func : m.functions)
        {
            w.write_compressed(func);
        }
    }
    if (!m.data.empty())
    {
        w.write(section_type::data_count);
        auto section_size = size_size(m.data);
        w.write_compressed(section_size);
        w.write_compressed(csize(m.data));
    }
    if (!m.code.empty())
    {
        w.write(section_type::code);
        auto section_size = size_size(m.code);
        for (auto& c : m.code)
        {
            auto size = c.size();
            section_size += size + static_cast<u32>(size_uleb128(size));
        }
        w.write_compressed(section_size);
        w.write_compressed(csize(m.code));
        for (auto& c : m.code)
        {
            w.write_compressed(c.size());
            w.write_compressed(csize(c.locals));

            for (auto& [count, type] : c.locals)
            {
                w.write_compressed(count);
                w.write(type);
            }

            for (auto& op : c.instructions)
                w.write(op);

            w.write("\x0B"sv);
        }
    }

    if (!m.data.empty())
    {
        w.write(section_type::data);
        auto section_size = size_size(m.data);

        for (auto& d : m.data)
        {
            auto size = csize(d.data);
            section_size += size + static_cast<u32>(size_uleb128(size)) + static_cast<u32>(size_uleb128(d.type));
        }

        w.write_compressed(section_size);
        w.write_compressed(csize(m.data));

        for (auto& d : m.data)
        {
            w.write_compressed(d.type);
            switch (d.type)
            {
            case 1:
                w.write_compressed(csize(d.data));
                for (auto& data : d.data)
                    w.write(data);
                break;
            default:
                break;
            }
        }
    }
}