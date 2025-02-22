import wumbo from "./wumbo.mjs";

export const newInstance = async (override) => {
  const instantiateBuffer = async (buffer) => {
    const { module, instance } = await WebAssembly.instantiate(
      buffer,
      importObject,
    );
    const { start } = instance.exports;
    return start;
  };

  const importObject = {
    print: {
      value: (arg) => {
        console.log(arg);
      },
      string: (arg) => {
        arg = new TextDecoder().decode(arg);
        console.log(arg);
      },
      array: (size) => new Uint8Array(size),
      set_array: (array, index, value) => (array[index] = value),
    },
    load: {
      load: instantiateBuffer,
    },
    string: {
      to_int: () => {},
      to_float: () => {},
    },
  };

  if (override) {
    Object.entries(importObject).forEach(([key, value]) => {
      importObject[key] = { ...value, ...override[key] };
    });
  }

  const instance = await wumbo();

  const load_lua = instance.cwrap("load_lua", "number", ["array", "number"]);
  const clean_up = instance.cwrap("clean_up", "void", ["number"]);
  const get_size = instance.cwrap("get_size", "number", ["number"]);
  const get_data = instance.cwrap("get_data", "number", ["number"]);
  const get_wat = instance.cwrap("get_wat", "string", ["number"]);
  const get_source_map = instance.cwrap("get_source_map", "string", ["number"]);

  return async (txt) => {
    const bytes = new TextEncoder().encode(txt);
    const result = load_lua(bytes, bytes.length);
    const wasm_bytes = get_data(result);
    const wasm_size = get_size(result);
    const wat = get_wat(result);

    const buffer = instance.HEAPU8.subarray(wasm_bytes, wasm_bytes + wasm_size);
    const start = await instantiateBuffer(buffer);
    clean_up(result);
    return [start, wat];
  };
};
