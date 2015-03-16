include_directories (
  ${GMOCK_INCLUDE_DIRECTORY}
  ${JSONCPP_INCLUDE_DIRECTORY}
)

set(LIBRARIES
    gmock 
    jsoncpp
)

set (SOURCES
  main.cc
  json_reader_test.cc
)

add_library("test_JSONCPPTest" ${SOURCES})
create_test("test_JSONCPP" "${SOURCES}" "${LIBRARIES}")