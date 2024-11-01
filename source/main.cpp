#include "ast.hpp"
#include "lua2wasm.hpp"
#include "wasm.hpp"

#include <ostream>
#include <string_view>

bool parse_file(std::string_view path, ast::block& state);
bool parse_string(std::string_view string, ast::block& state);
void to_stream_bin(std::ostream& f, const wasm::mod& m);
void to_stream_text(std::ostream& f, const wasm::mod& m);

#include <sstream>
#include <iostream>

#if defined(__EMSCRIPTEN__)
#include <emscripten.h>
EMSCRIPTEN_KEEPALIVE
extern "C" void load_lua(std::string data)
{
    std::cout << data.size() << "\n";

    ast::block chunk;

    parse_string(data, chunk);
    //wasm::mod result = wumbo::compile(chunk);

    //std::ostringstream stream{std::ios_base::binary | std::ios_base::out};
    //to_stream_bin(stream, result);
}

int main()
{
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
