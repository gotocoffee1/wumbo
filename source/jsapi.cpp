#include "ast.hpp"
#include "lua2wasm.hpp"
#include "wasm.hpp"

#include <emscripten.h>

namespace wumbo
{
bool parse_string(std::string_view string, ast::block& state);

struct result
{
    wasm::mod mod;
    std::unique_ptr<const void, deleter> data;
    size_t size;
    c_str source_map;
    c_str wat;
    std::string error;
};
} // namespace wumbo

extern "C"
{
    using namespace wumbo;
    EMSCRIPTEN_KEEPALIVE result* load_lua(const char* str, size_t size, uint32_t optimize)
    {
        auto res = new result{};
        try
        {
            ast::block chunk;
            parse_string(std::string_view{str, size}, chunk);
            res->mod = wumbo::compile(chunk, optimize, true);
        }
        catch (const std::exception& e)
        {
            res->error = e.what();
        }

        return res;
    }

    EMSCRIPTEN_KEEPALIVE void clean_up(result* ptr)
    {
        delete ptr;
    }

    EMSCRIPTEN_KEEPALIVE size_t get_size(result* ptr)
    {
        if (!ptr->data)
        {
            auto [data, size, source_map] = to_bin(ptr->mod);
            ptr->data = std::move(data);
            ptr->size = size;
            ptr->source_map = std::move(source_map);
        }
        return ptr->size;
    }

    EMSCRIPTEN_KEEPALIVE const void* get_data(result* ptr)
    {
        if (!ptr->data)
        {
            auto [data, size, source_map] = to_bin(ptr->mod);
            ptr->data = std::move(data);
            ptr->size = size;
            ptr->source_map = std::move(source_map);
        }
        return ptr->data.get();
    }

    EMSCRIPTEN_KEEPALIVE const char* get_source_map(result* ptr)
    {
        if (!ptr->source_map)
        {
            auto [data, size, source_map] = to_bin(ptr->mod);
            ptr->data = std::move(data);
            ptr->size = size;
            ptr->source_map = std::move(source_map);
        }
        return ptr->source_map.get();
    }

    EMSCRIPTEN_KEEPALIVE const char* get_wat(result* ptr, int format)
    {
        ptr->wat = to_txt(ptr->mod, static_cast<wat>(format));
        return ptr->wat.get();
    }

    EMSCRIPTEN_KEEPALIVE const char* get_error(result* ptr)
    {
        return ptr->error.c_str();
    }
}
