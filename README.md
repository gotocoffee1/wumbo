# wumbo

[wumbo](https://www.youtube.com/watch?v=--hsVknT1c0) is a [Lua](https://www.lua.org/) to [WebAssembly](https://webassembly.org/) compiler.

## WIP

🚧 The project is still work in progress and neither stable nor recommended for production. See [TODO](##TODO) 🚧

## Building

requirements:

- [C++17 compiler](https://en.cppreference.com/w/cpp/compiler_support/17) 
- [cmake](https://cmake.org/)
- [ninja](https://ninja-build.org/) (not required but recommended)

## TODO

- 🚧 goto/lable
- 🔜 good table implementation
- 🔜 optimize upvalues
- 🔜 check for tail calls
- 🚧 method calls

### Libs

- 🚧 basic lib
- 🔜 modules lib
- 🔜 string lib
- 🔜 utf8 lib
- 🔜 table lib
- 🔜 math lib
- ❓ coroutine (might need [this](https://github.com/WebAssembly/js-promise-integration/blob/main/proposals/js-promise-integration/Overview.md))

no prio:

- 🛑 debug lib
- 🛑 io lib
- 🛑 os lib

### Etc

- 🛑 rebuild Lua C-API for Emscripten

### Needs WebAssembly Feature Extension

- 🛑 [Weak Tables](https://www.lua.org/manual/5.3/manual.html#2.5.2)
- 🛑 [Garbage-Collection Metamethods tables](https://www.lua.org/manual/5.3/manual.html#2.5.1)

There is at least a [Post-MVP](https://github.com/WebAssembly/gc/blob/main/proposals/gc/Post-MVP.md#weak-references) for these. (Also [this](https://github.com/WebAssembly/gc/issues/77))

## License

wumbo is licensed under the MIT License, see [LICENSE](LICENSE) for more information.
