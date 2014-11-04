set(appMain_DIR ${CMAKE_SOURCE_DIR}/src/appMain)
include_directories (
  ${LOG4CXX_INCLUDE_DIRECTORY}
  ${GMOCK_INCLUDE_DIRECTORY}
  ${COMPONENTS_DIR}/connection_handler/include
  ${COMPONENTS_DIR}/protocol_handler/include
  ${COMPONENTS_DIR}/utils/include
  ${COMPONENTS_DIR}/config_profile/include
  ${COMPONENTS_DIR}/security_manager/test/include
  ${COMPONENTS_DIR}/security_manager/include
  )

set (LIBRARIES
    gtest
    gtest_main
    gmock
    gmock_main
    ProtocolHandler
    connectionHandler
    Utils
    ConfigProfile
    ProtocolLibrary   
)


set (SOURCES
  connection_handler_impl_test.cc
  connection_test.cc
  heart_beat_monitor_test.cc
  main.cc
)

 file(COPY ${appMain_DIR}/smartDeviceLink.ini DESTINATION "./")


create_test("test_connection_handler" "${SOURCES}" "${LIBRARIES}")

add_library("test_connection_handler_test" ${SOURCES})
