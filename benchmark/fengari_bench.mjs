import fengari from "fengari";
import { benchmark } from "./benchmark.mjs";

const luaconf = fengari.luaconf;
const lua = fengari.lua;
const lauxlib = fengari.lauxlib;
const lualib = fengari.lualib;

export const fengari_bench = benchmark({
    name: "fengari",
    compile: (data) => {
        const L = lauxlib.luaL_newstate();
        lualib.luaL_openlibs(L);
        lauxlib.luaL_loadstring(L, data);
        return L;
    },
    run: (L) => {
        lua.lua_call(L, 0, 0);
        lua.lua_close(L);
    },
});