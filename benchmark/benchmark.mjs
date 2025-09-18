import { argv } from "process";
import fs from "fs/promises";

const [filename, mode, iterations] = argv.slice(2, 5);

export const benchmark = async ({ init, compile, run, cleanup }) => {

    const [data, state] = await Promise.all([fs.readFile(filename), init()]);
    if (mode === "run") {
        const result = await compile(state, data);
        for (let i = 0; i < iterations; ++i) {
            await run(state, result);
        }
    } else {
        for (let i = 0; i < iterations; ++i) {
            await compile(state, data);
        }
    }
    await cleanup(state);
}
