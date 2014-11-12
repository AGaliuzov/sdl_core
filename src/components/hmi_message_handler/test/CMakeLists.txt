include_directories (
  ${LOG4CXX_INCLUDE_DIRECTORY}
  ${GMOCK_INCLUDE_DIRECTORY}
  ${COMPONENTS_DIR}/smart_objects/include
  ${COMPONENTS_DIR}/hmi_message_handler/include
  ${COMPONENTS_DIR}/utils/include
  ${JSONCPP_INCLUDE_DIRECTORY}
  ${CMAKE_BINARY_DIR}
  ${CMAKE_SOURCE_DIR}/src/3rd_party/dbus-1.7.8
  ${CMAKE_SOURCE_DIR}/src/3rd_party/dbus-1.7.8/dbus/
)

#include_directories (
#  ${CMAKE_SOURCE_DIR}/src/3rd_party-static/gmock-1.7.0/include
#  ${CMAKE_SOURCE_DIR}/src/3rd_party-static/gmock-1.7.0/gtest/include
#  ${CMAKE_SOURCE_DIR}/src/3rd_party/dbus-1.7.8
#  ${CMAKE_SOURCE_DIR}/src/3rd_party/dbus-1.7.8/dbus/
#)
set (LIBRARIES
    DBus
    gtest
    gtest_main
    gmock
    gmock_main
    Utils
    ProtocolHandler
    ApplicationManager
    SmartObjects
    jsoncpp
    HMIMessageHandler
    ${RTLIB}
    ProtocolLibrary
)

set(SOURCES
${COMPONENTS_DIR}/hmi_message_handler/test/mock_subscriber.cc
${COMPONENTS_DIR}/hmi_message_handler/test/test_dbus_message_adapter.cc
${COMPONENTS_DIR}/hmi_message_handler/test/test_mqueue_adapter.cc #ok

)          

create_test("test_hmi_message_handler" "${SOURCES}" "${LIBRARIES}")


