import { newInstance } from "./wumbo.mjs";
import fs from "fs/promises";

const load = await newInstance({ optimize: true });
console.time("compile");
const data = await fs.readFile("init.lua");
const [f, wat] = await load(data);
console.timeEnd("compile");
console.time("run");
f();
console.timeEnd("run");
