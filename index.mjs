import fs from "fs/promises";
import { argv, env } from "process";

const { newInstance, format } = await import(env.WUMBO_PATH);

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
