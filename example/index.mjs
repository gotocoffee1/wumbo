import { newInstance, format } from "../wumbo.mjs";
import fs from "fs/promises";
import { argv } from "process";

const arg = argv.slice(2);

const load = await newInstance({ optimize: true, format: format.none });
console.time("compile");
const data = await fs.readFile(arg[0]);
const [f, wat] = await load(data);
console.log(wat);
console.timeEnd("compile");
console.time("run");
console.log(await f(1n));
console.timeEnd("run");
