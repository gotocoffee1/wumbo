const instantiateBuffer = async (buffer, importObject) => {
  const { module, instance } = await WebAssembly.instantiate(
    buffer,
    importObject,
  );
  return instance.exports;
};

const makeImportObject = (override) => {
  const bufToStr = (buf) => new TextDecoder().decode(buf);
  const strToBuf = (str) => new TextEncoder().encode(str);
  const importObject = {
    load: {
      load: instantiateBuffer,
    },
    native: {
      stdout: (str) => console.log(bufToStr(str)),
      stderr: (str) => console.error(bufToStr(str)),
      toNum: (str) => Number(bufToStr(str)),
      toInt: (str) => BigInt(Number(bufToStr(str))),
      toString: (num) => strToBuf(num.toString()),
      pow: (base, exponent) => Math.pow(base, exponent),
    },
    buffer: {
      new: (size) => new Uint8Array(size),
      set: (array, index, value) => (array[index] = value),
      get: (array, index) => array[index],
      size: (array) => array.length,
    },
  };

  if (override) {
    Object.entries(importObject).forEach(([key, value]) => {
      importObject[key] = { ...value, ...override[key] };
    });
  }
  return importObject;
};
const instance = await wumbo();

const load_lua = instance.cwrap("load_lua", "number", [
  "array",
  "number",
  "number",
  "number",
]);
const generate_runtime = instance.cwrap("generate_runtime", "number", [
  "number",
]);
const clean_up = instance.cwrap("clean_up", "void", ["number"]);
const get_size = instance.cwrap("get_size", "number", ["number"]);
const get_data = instance.cwrap("get_data", "number", ["number"]);
const get_wat = instance.cwrap("get_wat", "string", ["number", "number"]);
const get_source_map = instance.cwrap("get_source_map", "string", ["number"]);
const get_error = instance.cwrap("get_error", "string", ["number"]);

const convertResult = async (result, format, importObject) => {
  const error = get_error(result);
  if (!error) {
    const wasm_bytes = get_data(result);
    const wasm_size = get_size(result);
    const wat = format != undefined ? get_wat(result, format) : undefined;

    const buffer = instance.HEAPU8.subarray(wasm_bytes, wasm_bytes + wasm_size);
    const exports = await instantiateBuffer(buffer, importObject);
    clean_up(result);
    return [exports, wat];
  } else {
    clean_up(result);
    throw new Error(error);
  }
};

const runtime = async (optimize, format, importObject) => {
  return await convertResult(generate_runtime(optimize), format, importObject);
};

const load = async (txt, importObject, optimize, format, standalone) => {
  const bytes = new TextEncoder().encode(txt);
  const [exports, wat] = await convertResult(
    load_lua(bytes, bytes.length, optimize, standalone),
    format,
    importObject,
  );
  return [exports.start, wat];
};

export const newInstance = async ({
  override = undefined,
  optimize = true,
  format = undefined,
  standalone = false,
} = {}) => {
  const importObject = makeImportObject(override);
  if (!standalone) {
    const [exports, wat] = await runtime(optimize, format, importObject);
    importObject.runtime = exports;
  }

  return async (txt) => {
    return await load(txt, importObject, optimize, format, standalone);
  };
};
