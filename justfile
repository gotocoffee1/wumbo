build CONFIG="Release":
    cmake --build --preset wasm32-emscripten-clang-{{CONFIG}}
test CONFIG="Release":
    ctest --preset wasm32-emscripten-clang-{{CONFIG}}
benchmark CONFIG="Release":
    cmake --build --preset wasm32-emscripten-clang-{{CONFIG}} --target benchmark

