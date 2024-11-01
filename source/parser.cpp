#include "parser.hpp"

#include "action.hpp"


bool parse_file(std::string_view path, ast::block& state)
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
