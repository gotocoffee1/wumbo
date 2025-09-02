include(FetchContent)

FetchContent_Declare(
  lua
  EXCLUDE_FROM_ALL
  GIT_REPOSITORY https://github.com/gotocoffee1/lua-cmake.git
  GIT_TAG c3e870ded0ab60b842a28caa86b465adacd777fd)

FetchContent_MakeAvailable(lua)

add_executable(lua_run main.c)
target_link_libraries(lua_run PRIVATE lua::static)

if (EMSCRIPTEN)
  # target_compile_options(lua PUBLIC
  # -fwasm-exceptions)
  target_link_options(lua_run PUBLIC
    #"SHELL:-s STANDALONE_WASM"
    #"SHELL:-s ASSERTIONS=2"
    #"SHELL:-s ALLOW_MEMORY_GROWTH"
    #"SHELL:-s  EXIT_RUNTIME=0"
    "SHELL:-sEXPORTED_RUNTIME_METHODS=cwrap"
    "SHELL:-sMODULARIZE=1"
    "SHELL:-sEXPORT_ES6=1"
    "SHELL:-sEXPORT_NAME=$<TARGET_NAME:lua_run>"
    --no-entry)
    # -fwasm-exceptions)
endif()
