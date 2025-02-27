import { newInstance } from "./wumbo.mjs";

const load = await newInstance();
console.log(load);
//console.time("lua");
const [f] = await load('print("test")');
//console.timeEnd("lua");
console.log(f);
//f();
