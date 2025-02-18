import { newInstance } from "./load.mjs";

const load = await newInstance();
//console.time("lua");
const f = await load('print("test")');
//console.timeEnd("lua");
f();
