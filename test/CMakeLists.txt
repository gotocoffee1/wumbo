
file(GLOB test_files "*.lua")
foreach(test_file ${test_files})
  cmake_path(GET test_file STEM test_name)
  add_test(NAME ${test_name}
  COMMAND node ${CMAKE_CURRENT_SOURCE_DIR}/index.mjs $<TARGET_FILE_DIR:wumbo> ${test_file})
endforeach()

