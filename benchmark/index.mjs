

import fs from "fs/promises";
import { argv } from "process";

const arg = argv.slice(2);
const data = await fs.readFile(arg[0]);

import { lua_bench } from "./lua_bench.mjs";
import { wumbo_bench } from "./wumbo_bench.mjs";
import { fengari_bench } from "./fengari_bench.mjs";

console.log("-----");
await wumbo_bench(data);
console.log("-----");
await lua_bench(data);
console.log("-----");
await fengari_bench(data);
console.log("-----");
