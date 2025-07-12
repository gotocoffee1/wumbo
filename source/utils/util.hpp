#pragma once

namespace wumbo
{
template<typename... Func>
struct overload : Func...
{
    using Func::operator()...;
};

template<typename... Func>
overload(Func...) -> overload<Func...>;

template<typename T, typename Tuple>
struct tuple_index;

template<typename T, typename... Types>
struct tuple_index<T, std::tuple<T, Types...>> : std::integral_constant<std::size_t, 0>
{
};

template<typename T, typename U, typename... Types>
struct tuple_index<T, std::tuple<U, Types...>> : std::integral_constant<std::size_t, 1 + tuple_index<T, std::tuple<Types...>>::value>
{
};

// Variable template for easier usage
template<typename T, typename Tuple>
constexpr std::size_t tuple_index_v = tuple_index<T, Tuple>::value;
} // namespace wumbo
