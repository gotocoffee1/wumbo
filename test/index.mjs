import { newInstance } from "../wumbo.mjs";
import fs from "fs/promises";
import path from "path";
import { execFile } from "child_process";
import { argv } from "process";
import util from "util";

const exec = util.promisify(execFile);

const bufToStr = (buf) => new TextDecoder().decode(buf);
const strToBuf = (str) => new TextEncoder().encode(str);
let buffer = new Uint8Array();
const load = await newInstance({
  optimize: true,
  override: {
    native: {
      //stdout: (arg) => process.stdout.write(arg),
      stdout: (arg) => (buffer = new Uint8Array([...buffer, ...arg])),
    },
  },
});

await Promise.all(
  (
    await Promise.all(
      (
        await Promise.all(
          // (await fs.readdir("."))
          argv
            .filter((f) => path.extname(f) === ".lua")
            .map(async (testFile) => {
              const data = await fs.readFile(testFile);
              const [func, wat] = await load(data);
              return [func, testFile];
            }),
        )
      ).map(async ([func, testFile]) => {
        func();
        const result = buffer;
        buffer = new Uint8Array();

        const { stdout, stderr } = await exec("lua", [testFile], {
          encoding: "buffer",
        });
        return [result, stdout, path.basename(testFile, ".lua")];
      }),
    )
  ).map(async ([result, expected, testName]) => {
    if (Buffer.compare(result, expected) === 0) {
      //console.log(`[${testName}] success`);
    } else {
      const file1 = `${testName}.wumbo.txt`;
      const file2 = `${testName}.lua.txt`;
      await Promise.all([
        fs.writeFile(file1, result),
        fs.writeFile(file2, expected),
      ]);
      process.exitCode = 1;
      console.error(`[${testName}] failed`);
      //console.error(`"${Buffer.from(result)}" vs "${expected}"`);
      execFile("diff", [file1, file2], {}, (error, stdout, stderr) =>
        console.error(stdout),
      );
    }
  }),
);
