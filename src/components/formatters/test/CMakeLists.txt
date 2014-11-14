include_directories (
  ${LOG4CXX_INCLUDE_DIRECTORY}
  ${GMOCK_INCLUDE_DIRECTORY}
  ${COMPONENTS_DIR}/smart_objects/include
  ${COMPONENTS_DIR}/formatters/include
  ${COMPONENTS_DIR}/utils/include
  ${JSONCPP_INCLUDE_DIRECTORY}
  ${CMAKE_BINARY_DIR}
)

set (LIBRARIES
    gmock
    gmock_main
    gtest
    gtest_main  
    SmartObjects
    formatters
    jsoncpp
)

set(SOURCES
${COMPONENTS_DIR}/formatters/test/generic_json_formatter_test.cc
${COMPONENTS_DIR}/formatters/test/main.cc
)

create_test("test_generic_json_formatter" "${SOURCES}" "${LIBRARIES}")
