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
}
