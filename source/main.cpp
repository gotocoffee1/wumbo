#include "ast.hpp"
#include "lua2wasm.hpp"
#include "wasm.hpp"

#include <fstream>
#include <iostream>

bool parse_file(std::string_view path, ast::block& state);
void to_stream_bin(std::ostream& f, const wasm::mod& m);
void to_stream_text(std::ostream& f, const wasm::mod& m);

int main(int argc, char** argv)
{
    for (int i = 1; i < argc; ++i)
    {
        try
        {
            ast::block chunk;
            const auto r = parse_file(argv[i], chunk);
            ast::printer p{std::cout};
            p(chunk);

            wasm::mod result = lua2wasm::compile(chunk);


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