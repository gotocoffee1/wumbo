import { env } from "process";
import { benchmark } from "./benchmark.mjs";

const { newInstance, format } = await import(env.WUMBO_PATH);

await benchmark({
  init: async () => await newInstance({ optimize: true, format: format.none }),
  compile: async (load, data) => (await load(data))[0],
  run: (_, f) => f(),
  cleanup: () => {},
});
