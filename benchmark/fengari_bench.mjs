import fengari from "fengari";
import { benchmark } from "./benchmark.mjs";

const luaconf = fengari.luaconf;
const lua = fengari.lua;
const lauxlib = fengari.lauxlib;
const lualib = fengari.lualib;

await benchmark({
    init: () => {
        const L = lauxlib.luaL_newstate();
        lualib.luaL_openlibs(L);
        lua.lua_pushnil(L);
        return L;
    },
    compile: (L, data) => {
        lua.lua_pop(L, 1);
        lauxlib.luaL_loadstring(L, data);
    },
    run: (L, _) => {
        lua.lua_pushvalue(L, -1);
        lua.lua_call(L, 0, 0);
    },
    cleanup: (L) => {
        lua.lua_close(L);
    },
});
