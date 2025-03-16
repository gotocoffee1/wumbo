import { newInstance } from "./wumbo.mjs";

const load = await newInstance({ optimize: false });
console.log(load);
//console.time("lua");
const [f, wat] = await load('print("test")');
console.log(wat);
//console.timeEnd("lua");
f();
