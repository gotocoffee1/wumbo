import fs from "fs/promises";
import os from "os";
import path from "path";
import { exec } from "child_process";
import process from "process";

const time_cmd_ = (cmd, env) =>
  new Promise((resolve, reject) => {
    exec(
      `time -f"[%U, %M]" ${cmd}`,
      { cwd: import.meta.dirname, env: { ...process.env, ...env } },
      (error, stdout, stderr) => {
        console.log(stdout);
        if (error) {
          reject(error);
        } else {
          resolve(JSON.parse(stderr));
        }
      },
    );
  });

const time_cmd = (runner, env, file, mode, iterations) => {
  return time_cmd_(`${runner} "${file}" ${mode} ${iterations}`, env);
};

const iterationsCompile = 50;
const iterationsRun = 50;
const wumbo = time_cmd.bind(
  null,
  path.join(path.dirname(import.meta.dirname), "wumbo"),
  { WUMBO_INIT_PATH: "wumbo_bench.mjs" },
);
const lua_wasm = time_cmd.bind(
  null,
  `node ${path.join(path.dirname(process.env.WUMBO_PATH), "lua.mjs")} init.lua`,
  {},
);
const fengari = time_cmd.bind(null, "node fengari_bench.mjs", {});
const lua = time_cmd.bind(null, "lua init.lua", {});
const luajit = time_cmd.bind(null, "luajit init.lua", {});

const bench = async (name, cmd, algo) => {
  console.time(`[${name}]`);
  const [compileTime, compileMem] = await cmd(
    algo,
    "compile",
    iterationsCompile,
  );
  console.timeLog(`[${name}]`, "(compile)");
  const [runTime, runMem] = await cmd(algo, "run", iterationsRun);
  console.timeEnd(`[${name}]`);
  const comp = (compileTime * 1000) / iterationsCompile;
  const run = (runTime * 1000) / iterationsRun;
  return [name, comp, run, compileMem, runMem];
};

const wumbo_bench = bench.bind(null, "wumbo", wumbo);
const lua_wasm_bench = bench.bind(null, "lua_wasm", lua_wasm);
const fengari_bench = bench.bind(null, "fengari", fengari);
const lua_bench = bench.bind(null, "lua", lua);
const luajit_bench = bench.bind(null, "luajit", luajit);

const algos = [
  "fibonacci.lua",
  // "heapsort.lua",
  "fixpoint-fact.lua",
];

const result = [];

for (const algo of algos) {
  const p = path.join("algo", algo);
  const title = path.basename(algo, ".lua");
  console.log(`~~${title}~~`);

  result.push({
    title: title,
    data: [
      await wumbo_bench(p),
      await lua_wasm_bench(p),
      await fengari_bench(p),
      await lua_bench(p),
      await luajit_bench(p),
    ],
  });
}

const stdout = (cmd) =>
  new Promise((resolve, reject) => {
    exec(cmd, (error, stdout, stderr) => {
      if (error) {
        reject(error);
      } else {
        resolve(stdout);
      }
    });
  });

const [luaVersion, luajitVersion] = await Promise.all([
  stdout("lua -v"),
  stdout("luajit -v"),
]);

await fs.writeFile(
  "bench_result.json",
  JSON.stringify({
    version: process.version,
    v8: process.versions.v8,
    platform: os.platform(),
    arch: os.arch(),
    cpu: os.cpus()[0].model,
    lua: luaVersion,
    luajit: luajitVersion,
    result: result,
  }),
);
