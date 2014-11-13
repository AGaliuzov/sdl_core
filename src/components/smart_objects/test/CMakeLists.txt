add_definitions(-DUNIT_TESTS)

# --- SchemaItems
add_subdirectory(./SchemaItem)

include_directories (
  ${LOG4CXX_INCLUDE_DIRECTORY}
  ${COMPONENTS_DIR}/utils/include
  ${GMOCK_INCLUDE_DIRECTORY}
  ${COMPONENTS_DIR}/smart_objects/include
  ${COMPONENTS_DIR}/formatters/include/
  ${JSONCPP_INCLUDE_DIRECTORY}
)

set (LIBRARIES
    gtest
    gtest_main
    gmock
    gmock_main
    SmartObjects
    formatters
    jsoncpp
    ${RTLIB}
    ProtocolLibrary
)

set (SOURCES
  ${COMPONENTS_DIR}/smart_objects/test/SmartObjectDraftTest.cc
  ${COMPONENTS_DIR}/smart_objects/test/SmartObjectInvalidTest.cc
  ${COMPONENTS_DIR}/smart_objects/test/SmartObjectStressTest.cc
  ${COMPONENTS_DIR}/smart_objects/test/SmartObjectUnitTest.cc
  ${COMPONENTS_DIR}/smart_objects/test/TSharedPtrTest.cc
  ${COMPONENTS_DIR}/smart_objects/test/main.cc
)

add_library(test_smart_object ${SOURCES})
create_test(test_smart_object_test "${SOURCES}" "${LIBRARIES}")

create_test("test_smart_object_convertion_time_test" "./SmartObjectConvertionTimeTest.cc" "${LIBRARIES}")
create_test("test_smart_object_performance_test" "./smart_object_performance_test.cc" "${LIBRARIES}")
create_test("test_map_performance_test" "./map_performance_test.cc" "${LIBRARIES}")
