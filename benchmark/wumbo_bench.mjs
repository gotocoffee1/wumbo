import { newInstance, format } from "../wumbo.mjs";
import { benchmark } from "./benchmark.mjs";

await benchmark({
    init: async () => await newInstance({ optimize: true, format: format.none }),
    compile: async (load, data) => (await load(data))[0],
    run: (_, f) => f(),
    cleanup: () => { },
});
