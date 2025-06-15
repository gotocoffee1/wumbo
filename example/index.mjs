import { newInstance } from "../wumbo.mjs";
import fs from "fs/promises";
import { argv } from "process";

const arg = argv.slice(2);
const load = await newInstance({ optimize: true, format: undefined });
console.time("compile");
const data = await fs.readFile(arg[0]);
const [f, wat] = await load(data);
console.log(wat);
console.timeEnd("compile");
console.time("run");
await f();
console.timeEnd("run");
