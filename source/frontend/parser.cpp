#include "parser.hpp"

#include "action.hpp"

namespace wumbo
{
bool parse_stream(std::istream& stream, ast::block& state)
{
    TAO_PEGTL_NAMESPACE::istream_input in(stream, 40, "");
    //if (const auto root = TAO_PEGTL_NAMESPACE::parse_tree::parse<lua53::grammar, lua53::selector>(in, state))
    //{
    //    TAO_PEGTL_NAMESPACE::parse_tree::print_dot(std::cout, *root);
    //    return 0;
    //}

    return TAO_PEGTL_NAMESPACE::parse<lua53::grammar, lua53::action>(in, state);
}

bool parse_file(const std::filesystem::path& path, ast::block& state)
{
    TAO_PEGTL_NAMESPACE::file_input in(path);
    //if (const auto root = TAO_PEGTL_NAMESPACE::parse_tree::parse<lua53::grammar, lua53::selector>(in, state))
    //{
    //    TAO_PEGTL_NAMESPACE::parse_tree::print_dot(std::cout, *root);
    //    return 0;
    //}

    return TAO_PEGTL_NAMESPACE::parse<lua53::grammar, lua53::action>(in, state);
}



bool parse_string(std::string_view string, ast::block& state)
{
    TAO_PEGTL_NAMESPACE::memory_input in(string, "");
    //if (const auto root = TAO_PEGTL_NAMESPACE::parse_tree::parse<lua53::grammar, lua53::selector>(in, state))
    //{
    //    TAO_PEGTL_NAMESPACE::parse_tree::print_dot(std::cout, *root);
    //    return 0;
    //}

    return TAO_PEGTL_NAMESPACE::parse<lua53::grammar, lua53::action>(in, state);
}
}
