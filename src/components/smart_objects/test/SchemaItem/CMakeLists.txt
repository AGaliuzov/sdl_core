include_directories (
  ${COMPONENTS_DIR}/utils/include
  ${GMOCK_INCLUDE_DIRECTORY}
  ${COMPONENTS_DIR}/smart_objects/include
  ${COMPONENTS_DIR}/formatters/include/
  ${JSONCPP_INCLUDE_DIRECTORY}
)

# Libraries are the same for all SchemaItem tests
set (LIBRARIES
    gtest
    gtest_main
    gmock
    gmock_main
    SmartObjects
    formatters
    jsoncpp
)

set (SOURCES
  ${COMPONENTS_DIR}/smart_objects/test/SchemaItem/test_BoolSchemaItemTest.cc
  ${COMPONENTS_DIR}/smart_objects/test/SchemaItem/test_NumberSchemaItemTest.cc
  ${COMPONENTS_DIR}/smart_objects/test/SchemaItem/test_StringSchemaItemTest.cc
  ${COMPONENTS_DIR}/smart_objects/test/SchemaItem/test_ArraySchemaItemTest.cc
  ${COMPONENTS_DIR}/smart_objects/test/SchemaItem/test_CObjectSchemaItem.cc
  ${COMPONENTS_DIR}/smart_objects/test/SchemaItem/AlwaysTrueSchemaItemTest.cc
  ${COMPONENTS_DIR}/smart_objects/test/SchemaItem/AlwaysFalseSchemaItemTest.cc
)

add_library(test_smart_object_schema ${SOURCES})
create_test(test_smart_object_schema_test "${SOURCES}" "${LIBRARIES}")
