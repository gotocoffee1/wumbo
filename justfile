
PRESET := "wasm32-emscripten-clang"

configure:
    cmake --preset {{PRESET}}
build CONFIG="Release":
    cmake --build --preset {{PRESET}}-{{CONFIG}}
rebuild CONFIG="Release":
    cmake --build --clean-first --preset {{PRESET}}-{{CONFIG}}
test CONFIG="Release":
    ctest --preset {{PRESET}}-{{CONFIG}}
target TARGET CONFIG="Release":
    cmake --build --preset {{PRESET}}-{{CONFIG}} --target {{TARGET}}
clean CONFIG="Release": (target "clean" CONFIG)
benchmark CONFIG="Release": (target "benchmark" CONFIG)
example CONFIG="Release": (target "example" CONFIG)
install CONFIG="Release": (target "install" CONFIG)
pack CONFIG="Release":
    cpack --preset {{PRESET}}-{{CONFIG}}
