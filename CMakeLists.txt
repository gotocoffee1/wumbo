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
  "source/frontend/parser.cpp"
  "source/lua2wasm.cpp"
  "source/backend/wasm.cpp"
  "source/backend/locals.cpp"
  "source/backend/runtime/runtime.cpp"
  "source/backend/runtime/operator.cpp"
  "source/backend/runtime/table.cpp"
  "source/backend/assignment_call.cpp"
  "source/backend/expression.cpp"
  "source/backend/table.cpp"
  "source/backend/loop.cpp"
  "source/backend/basic_lib.cpp"
  "source/backend/operators.cpp")

add_library(wumbo::wumbo ALIAS wumbo)

find_package(pegtl CONFIG REQUIRED)
find_package(CLI11 CONFIG REQUIRED)
find_package(span-lite CONFIG REQUIRED)

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
  GIT_TAG 9b161a9bf93c26feb05c727a838444c42398ba78
  EXCLUDE_FROM_ALL
)

set(EMSCRIPTEN_ENABLE_WASM_EH ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(binaryen)

set(CMAKE_CXX_FLAGS ${temp})
set(CMAKE_C_FLAGS ${temp2})

target_compile_features(wumbo PUBLIC cxx_std_17)

target_link_libraries(wumbo
  PRIVATE
  nonstd::span-lite
  taocpp::pegtl
  binaryen)

target_include_directories(wumbo PRIVATE ${binaryen_SOURCE_DIR}/src)
target_include_directories(wumbo PUBLIC source)


if (EMSCRIPTEN)
  add_executable(wumbo_exe source/jsapi.cpp)
else()
  add_executable(wumbo_exe source/main.cpp)
endif()
target_link_libraries(wumbo_exe PUBLIC wumbo)

if (EMSCRIPTEN)
  set(CMAKE_EXECUTABLE_SUFFIX ".mjs")
  target_compile_options(wumbo PUBLIC
  -fwasm-exceptions)
  target_link_options(wumbo PUBLIC
  #"SHELL:-s STANDALONE_WASM"
  #"SHELL:-s ASSERTIONS=2"
  #"SHELL:-s ALLOW_MEMORY_GROWTH"
  #"SHELL:-s  EXIT_RUNTIME=0"
  "SHELL:-sEXPORTED_RUNTIME_METHODS=cwrap"
  "SHELL:-sMODULARIZE=1"
  "SHELL:-sEXPORT_ES6=1"
  "SHELL:-sEXPORT_NAME=$<TARGET_NAME:wumbo>"
  --extern-post-js "${CMAKE_CURRENT_LIST_DIR}/js/post.mjs"
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

#add_subdirectory(benchmark)

# ---- Install rules ----
if(NOT CMAKE_SKIP_INSTALL_RULES)
  #include(cmake/install-rules.cmake)
endif()

include(CTest)

if(BUILD_TESTING)
  add_subdirectory(test)
endif()

