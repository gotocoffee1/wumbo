#include "ast/ast.hpp"
#include "ast/analyze.hpp"
#include "ast/printer.hpp"
#include "backend/wasm.hpp"
#include "lua2wasm.hpp"

#include <ostream>
#include <string_view>

namespace wumbo
{
void to_stream_bin(std::ostream& f, const wasm::mod& m);
void to_stream_text(std::ostream& f, const wasm::mod& m);
} // namespace wumbo

#include <filesystem>
#include <fstream>
#include <iostream>

#include <CLI/CLI.hpp>

namespace wumbo
{
bool parse_stream(std::istream& stream, ast::block& state);
bool parse_file(const std::filesystem::path& path, ast::block& state);
} // namespace wumbo

enum class export_mode
{
    standalone,
    minimal,
    runtime,
};

int main(int argc, char** argv)
{
    using namespace wumbo;

    CLI::App app{"A Lua to WebAssembly compiler", "wumbo"};

    std::filesystem::path infile;
    std::filesystem::path outfile;
    export_mode mode  = export_mode::standalone;
    bool text         = false;
    uint32_t optimize = 0;

    std::map<std::string, export_mode> map{{"standalone", export_mode::standalone}, {"minimal", export_mode::minimal}, {"runtime", export_mode::runtime}};

    app.add_option("infile", infile, "input file")->default_str("stdin");
    app.add_option("-o,--outfile", outfile, "output file")->default_str("stdout");
    app.add_option("-m,--mode", mode, "standalone wasm file")
        ->transform(CLI::CheckedTransformer(map, CLI::ignore_case))
        ->capture_default_str();

    app.add_option("-O", optimize, "enable optimization")->capture_default_str();
    app.add_flag("-t,--text", text, "text format")->capture_default_str();
    CLI11_PARSE(app, argc, argv);

    try
    {
        ast::block chunk;
        if (mode != export_mode::runtime)
        {
            //std::ifstream instream(infile, std::ios::in | std::ios::binary | std::ios::failbit | std::ios::badbit);
            const auto r = parse_file(infile, chunk);
            ast::printer p{std::cout};
            p(chunk);
        }

        ast::analyzer{}(chunk);
        wasm::mod result = (mode == export_mode::runtime) ? wumbo::make_runtime(optimize) : wumbo::compile(chunk, optimize, mode == export_mode::standalone);
        {
            std::ofstream ofstream;
            if (!outfile.empty())
            {
                ofstream.open(outfile, std::ios_base::out | std::ios_base::binary);
                if (!ofstream.is_open())
                    throw std::system_error(errno, std::generic_category(), outfile.string());
            }
            std::ostream& ostream = outfile.empty() ? std::cout : ofstream;
            if (text)
                to_stream_text(ostream, result);
            else
                to_stream_bin(ostream, result);
        }
    }
    catch (const std::exception& e)
    {
        std::cout << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
