include(FetchContent)

FetchContent_Declare(
  lua
  EXCLUDE_FROM_ALL
  GIT_REPOSITORY https://github.com/gotocoffee1/lua-cmake.git
  GIT_TAG 54fc89ea4be7595adf0ecfce5fba8306f20c13e0)

FetchContent_MakeAvailable(lua)

if (EMSCRIPTEN)
  target_link_options(lua_static PUBLIC -lnoderawfs.js -lnodefs.js)
#   # target_compile_options(lua PUBLIC
#   # -fwasm-exceptions)
#   target_link_options(lua_run PUBLIC
#     # -fwasm-exceptions)
endif()

configure_file(index.html index.html @ONLY)
add_custom_target(benchmark COMMAND ${CMAKE_COMMAND} -E env WUMBO_PATH="$<TARGET_FILE:wumbo_exe>" node ${CMAKE_CURRENT_SOURCE_DIR}/index.mjs BYPRODUCTS "bench_result.json")
add_dependencies(benchmark wumbo_exe lua::interpreter)

