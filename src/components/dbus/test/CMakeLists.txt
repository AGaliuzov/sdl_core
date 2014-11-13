include_directories (
  ${LOG4CXX_INCLUDE_DIRECTORY}
  ${GMOCK_INCLUDE_DIRECTORY}
  ${COMPONENTS_DIR}
  ${CMAKE_BINARY_DIR}/src/components/
  ${COMPONENTS_DIR}/dbus/include/
  ${COMPONENTS_DIR}/utils/include/
  ${COMPONENTS_DIR}/smart_objects/include/
)

set (LIBRARIES
    gtest
    gtest_main
    gmock
    gmock_main
    DBus
    formatters
    SmartObjects
    Utils
)

set (SCHEMA_SOURCES
   ${COMPONENTS_DIR}/dbus/test/test_schema.cc
)

set (DBUS_ADAPTER_SOURCES
   ${COMPONENTS_DIR}/dbus/test/test_dbus_adapter.cc
)

create_test("test_DBusSchema" "${SCHEMA_SOURCES}" "${LIBRARIES}")
create_test("test_DBusAdapter" "${DBUS_ADAPTER_SOURCES}" "${LIBRARIES}")


