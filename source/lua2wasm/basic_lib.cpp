#include "compiler.hpp"

namespace wumbo
{

void compiler::open_basic_lib()
{
}

void compiler::setup_env()
{
    _func_stack.push_function(0);
    _func_stack.alloc_lua_local("_ENV", upvalue_type());
    set_var("_ENV", (*this)(table_constructor{}));


    // todo:
    //local _ENV = {}
    //_ENV._G = _ENV

}

} // namespace wumbo
