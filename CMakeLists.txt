cmake_minimum_required(VERSION 3.14)

project(
  wumbo
  VERSION 0.1.0
  DESCRIPTION "Short description"
  HOMEPAGE_URL "https://example.com/"
  LANGUAGES C CXX
)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CONFIG})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CONFIG})



add_library(wumbo
  "source/parser.cpp"
  "source/lua2wasm.cpp"
  "source/wasm.cpp"
  "source/lua2wasm/locals.cpp"
  "source/lua2wasm/assignment_call.cpp"
  "source/lua2wasm/expression.cpp"
  "source/lua2wasm/table.cpp"
  "source/lua2wasm/loop.cpp"
  "source/lua2wasm/basic_lib.cpp"
  "source/lua2wasm/operators.cpp")

add_library(wumbo::wumbo ALIAS wumbo)

find_package(CLI11 CONFIG REQUIRED)
find_package(pegtl REQUIRED)


set(temp ${CMAKE_CXX_FLAGS})
set(temp2 ${CMAKE_C_FLAGS})
string(REGEX REPLACE "/W[1-4]" "/W0" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
string(REGEX REPLACE "/W[1-4]" "/W0" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
string(REGEX REPLACE "/analyze:external-" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
string(REGEX REPLACE "/analyze" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
string(REGEX REPLACE "/showIncludes" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
string(REGEX REPLACE "/sdl" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
string(REGEX REPLACE "/std:c..17" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")


set(CMAKE_CXX_FLAGS "")
set(CMAKE_C_FLAGS "")

include(FetchContent)

FetchContent_Declare(
  binaryen 
  GIT_REPOSITORY https://github.com/WebAssembly/binaryen
  GIT_TAG dcc70bbfb16c2f8fce29dad94d80d1b78123655f
)


set(EMSCRIPTEN_ENABLE_WASM_EH ON CACHE BOOL "" FORCE)


FetchContent_GetProperties(binaryen)
if(NOT binaryen_POPULATED)
  FetchContent_Populate(binaryen)

  add_subdirectory(${binaryen_SOURCE_DIR} ${binaryen_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()


set(CMAKE_CXX_FLAGS ${temp})
set(CMAKE_C_FLAGS ${temp2})

target_compile_features(wumbo PUBLIC cxx_std_17)

target_link_libraries(wumbo
  PRIVATE
  taocpp::pegtl
  binaryen)

target_include_directories(wumbo PRIVATE ${binaryen_SOURCE_DIR}/src)
target_include_directories(wumbo PRIVATE source)


add_executable(wumbo_exe source/main.cpp)
target_link_libraries(wumbo_exe PUBLIC wumbo)

if (EMSCRIPTEN)
  target_compile_options(wumbo PUBLIC
  -fwasm-exceptions)
  target_link_options(wumbo PUBLIC
  #"SHELL:-s STANDALONE_WASM"
  #"SHELL:-s ASSERTIONS=2"
  #"SHELL:-s ALLOW_MEMORY_GROWTH"
  #"SHELL:-s  EXIT_RUNTIME=0"
  "SHELL:-sEXPORTED_RUNTIME_METHODS=cwrap"
  --no-entry
  -fwasm-exceptions)
else()
  target_link_libraries(wumbo_exe PUBLIC CLI11::CLI11)
endif()

set_target_properties(
  wumbo_exe
  PROPERTIES
  EXPORT_NAME wumbo
  OUTPUT_NAME wumbo)

# ---- Install rules ----
if(NOT CMAKE_SKIP_INSTALL_RULES)
  #include(cmake/install-rules.cmake)
endif()
