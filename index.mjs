import fs from "fs/promises";
import { argv, env } from "process";

const { newInstance, format } = await import(env.WUMBO_PATH);

const arg = argv.slice(2);

const [load, runtimeWat] = await newInstance({
  optimize: false,
  format: format.stack,
  standalone: false,
});
if (runtimeWat) console.log(runtimeWat);
console.time("compile");
const data = await fs.readFile(arg[0]);
const [f, wat] = await load(data);
if (wat) console.log(wat);
console.timeEnd("compile");
console.time("run");
try {
  console.log(await f(1n));
} catch (e) {
  console.error(e);
}
console.timeEnd("run");
