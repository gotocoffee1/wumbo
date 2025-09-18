include(FetchContent)

FetchContent_Declare(
  lua
  # EXCLUDE_FROM_ALL
  GIT_REPOSITORY https://github.com/gotocoffee1/lua-cmake.git
  GIT_TAG c3e870ded0ab60b842a28caa86b465adacd777fd)

FetchContent_MakeAvailable(lua)

if (EMSCRIPTEN)
  target_link_options(lua_static PUBLIC -lnoderawfs.js -lnodefs.js)
#   # target_compile_options(lua PUBLIC
#   # -fwasm-exceptions)
#   target_link_options(lua_run PUBLIC
#     # -fwasm-exceptions)
endif()
