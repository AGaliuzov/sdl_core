include_directories (
  ${LOG4CXX_INCLUDE_DIRECTORY}
  ${COMPONENTS_DIR}/dbus/include
  ${COMPONENTS_DIR}/dbus/src
  ${COMPONENTS_DIR}/rpc_base/include
  ${CMAKE_SOURCE_DIR}/src/3rd_party/dbus-1.7.8
  ${GMOCK_INCLUDE_DIRECTORY}
  ${JSONCPP_INCLUDE_DIRECTORY}
)

set (LIBRARIES
    gmock
    gmock_main
    gtest
    gtest_main
    jsoncpp
)

set (SOURCES
  rpc_base_json_test.cc
  rpc_base_test.cc
  main.cc
)

if (${HMI_DBUS_API})
  # Build dbus tests
  include_directories(${DBUS_INCLUDE_DIRS})
  set (LIBRARIES ${LIBRARIES} DBus)
  set (SOURCES ${SOURCES} rpc_base_dbus_test.cc)
endif ()

create_test("test_rpc_base" "${SOURCES}" "${LIBRARIES}")




