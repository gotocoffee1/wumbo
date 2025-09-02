import lua_run from "../lua_run.mjs";
import { benchmark } from "./benchmark.mjs";

const instance = await lua_run();

const load_lua = instance.cwrap("load_lua", "number", [
    "array",
    "number",
]);

const clean_up = instance.cwrap("clean_up", "void", ["number"]);
const run = instance.cwrap("run", "void", ["number"]);

export const lua_bench = benchmark({
    name: "lua",
    compile: (bytes) => load_lua(bytes, bytes.length),
    run: (state) => { run(state); clean_up(state); },
});
