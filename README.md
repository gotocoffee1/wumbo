# wumbo

[wumbo](https://www.youtube.com/watch?v=--hsVknT1c0) is a [Lua](https://www.lua.org/) to [WebAssembly](https://webassembly.org/) compiler.

## WIP

🚧 The project is still work in progress and neither stable nor recommended for production. 🚧

## TODO

- [ ] goto/lable
- [ ] good table implementation
- [ ] optimize upvalues
- [ ] check for tail calls
- [ ] method calls
- [ ] Lua std libs

### Needs WebAssembly Feature Extension

- [ ] [Weak Tables](https://www.lua.org/manual/5.3/manual.html#2.5.2)
- [ ] [Garbage-Collection Metamethods tables](https://www.lua.org/manual/5.3/manual.html#2.5.1)

There is at least a [Post-MVP](https://github.com/WebAssembly/gc/blob/main/proposals/gc/Post-MVP.md#weak-references) for these. (Also [this](https://github.com/WebAssembly/gc/issues/77))

## License

wumbo is licensed under the MIT License, see [LICENSE](LICENSE) for more information.
