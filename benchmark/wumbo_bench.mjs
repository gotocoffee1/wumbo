import { newInstance, format } from "../wumbo.mjs";
import { benchmark } from "./benchmark.mjs";

const load = await newInstance({ optimize: true, format: format.none });
export const wumbo_bench = benchmark({
    name: "wumbo",
    compile: async (data) => (await load(data))[0],
    run: (f) => f(),
});
