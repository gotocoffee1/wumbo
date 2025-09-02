#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
#include <stdio.h>

#include <emscripten.h>

EMSCRIPTEN_KEEPALIVE lua_State* load_lua(const char* str, size_t size)
{
    lua_State* L = luaL_newstate();
    luaL_openlibs(L);

    if (luaL_loadbuffer(L, str, size, "line") == LUA_OK)
    {
    }
    return L;
}

EMSCRIPTEN_KEEPALIVE void run(lua_State* L)
{
    lua_call(L, 0, 0);
}

EMSCRIPTEN_KEEPALIVE void clean_up(lua_State* L)
{
    lua_close(L);
}
