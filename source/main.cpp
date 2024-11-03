#include "ast.hpp"
#include "lua2wasm.hpp"
#include "wasm.hpp"

#include <ostream>
#include <string_view>

bool parse_file(std::string_view path, ast::block& state);
bool parse_string(std::string_view string, ast::block& state);
void to_stream_bin(std::ostream& f, const wasm::mod& m);
void to_stream_text(std::ostream& f, const wasm::mod& m);

#if defined(__EMSCRIPTEN__)
#include <emscripten.h>
extern "C"
{
    EMSCRIPTEN_KEEPALIVE result* load_lua(const char* str, size_t size)
    {
        ast::block chunk;

        parse_string(std::string_view{str, size}, chunk);
        wasm::mod wasm = wumbo::compile(chunk);

        return new result{to_stream_bin(wasm)};
    }

    EMSCRIPTEN_KEEPALIVE void clean_up(result* ptr)
    {
        delete ptr;
    }

    EMSCRIPTEN_KEEPALIVE size_t get_size(result* ptr)
    {
        return ptr->size;
    }

    EMSCRIPTEN_KEEPALIVE void* get_data(result* ptr)
    {
        return ptr->data.get();
    }
}

#else
#include <fstream>
#include <iostream>

int main(int argc, char** argv)
{
    for (int i = 1; i < argc; ++i)
    {
        std::cout << argv[i] << "\n";
        try
        {
            ast::block chunk;
            //const auto r = parse_string(argv[i], chunk);
            const auto r = parse_file(argv[i], chunk);
            ast::printer p{std::cout};
            p(chunk);

            wasm::mod result = wumbo::compile(chunk);

            std::ofstream f("out.wasm", std::ios::binary);
            to_stream_bin(f, result);
            std::ofstream f2("out.wat", std::ios::binary);
            to_stream_text(f2, result);
        }
        catch (const std::exception& e)
        {
            std::cout << e.what() << std::endl;
        }
    }
    return 0;
}

#endif
