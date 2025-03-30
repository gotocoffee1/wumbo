import { newInstance } from "./wumbo.mjs";
import fs from "fs/promises";

const load = await newInstance({ optimize: true });
//console.log(load);
//console.time("lua");
const data = await fs.readFile("init.lua");
const [f, wat] = await load(data);
//console.log(wat);
//console.timeEnd("lua");
f();
