import fs from "fs/promises";
import os from "os";
import path from "path";

import { lua_bench } from "./lua_bench.mjs";
import { wumbo_bench } from "./wumbo_bench.mjs";
import { fengari_bench } from "./fengari_bench.mjs";

const algos = [
    "fibonacci.lua",
    // "heapsort.lua",
    "fixpoint-fact.lua",
];

const result = [];

for (const algo of algos) {
    const data = await fs.readFile(algo);
    result.push({
        title: path.basename(algo, ".lua"),
        data: [
            await wumbo_bench(data),
            await lua_bench(data),
            await fengari_bench(data),
        ],
    });
}

await fs.writeFile("result.json", JSON.stringify({
    version: process.version,
    v8: process.versions.v8,
    platform: os.platform(),
    arch: os.arch(),
    cpu: os.cpus()[0].model,
    result: result,
}));
