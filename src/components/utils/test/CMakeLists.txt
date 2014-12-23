include_directories (
  ${CMAKE_SOURCE_DIR}/src/3rd_party-static/gmock-1.7.0/include
  ${CMAKE_SOURCE_DIR}/src/3rd_party-static/gmock-1.7.0/gtest/include)

set(testSources
  main.cc
  messagemeter_test.cc
  file_system_test.cc
  date_time_test.cc
  system_test.cc
  signals_linux_test.cc
  thread_validator_test.cc
  )

set(testLibraries
  gmock
  gtest
  Utils)

file(COPY testscript.sh DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
add_executable(utils_test ${testSources})
target_link_libraries(utils_test ${testLibraries})

