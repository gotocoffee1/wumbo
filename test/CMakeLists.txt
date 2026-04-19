
file(GLOB test_files "*.lua")
foreach(test_file ${test_files})
  cmake_path(GET test_file STEM test_name)
  add_test(NAME ${test_name}
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/../wumbo ${test_file})
  set_property(TEST ${test_name} PROPERTY ENVIRONMENT 
    "WUMBO_PATH=$<TARGET_FILE:wumbo_exe>"
    "WUMBO_INIT_PATH=${CMAKE_CURRENT_SOURCE_DIR}/index.mjs"
  )
endforeach()

