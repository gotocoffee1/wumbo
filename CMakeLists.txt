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

add_executable(wumbo source/main.cpp "source/parser.cpp" "source/lua2wasm.cpp" "source/wasm.cpp")
add_executable(wumbo::wumbo ALIAS wumbo)

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

include(FetchContent)

FetchContent_Declare(
  binaryen 
  GIT_REPOSITORY https://github.com/WebAssembly/binaryen
  GIT_TAG 37a86d558994415e722e7f62f5784b1a8b8b4832
)

FetchContent_MakeAvailable(binaryen)



set(CMAKE_CXX_FLAGS ${temp})
set(CMAKE_C_FLAGS ${temp2})

target_compile_features(wumbo PUBLIC cxx_std_17)

target_link_libraries(wumbo
  PUBLIC
  CLI11::CLI11
  taocpp::pegtl
  binaryen)

target_include_directories(wumbo PUBLIC ${binaryen_SOURCE_DIR}/src)

# ---- Install rules ----
if(NOT CMAKE_SKIP_INSTALL_RULES)
  #include(cmake/install-rules.cmake)
endif()
