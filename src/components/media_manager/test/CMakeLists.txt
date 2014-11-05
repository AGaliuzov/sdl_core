include_directories (
  ${LOG4CXX_INCLUDE_DIRECTORY}
  ${GMOCK_INCLUDE_DIRECTORY}
  ${COMPONENTS_DIR}/media_manager/include
  ${COMPONENTS_DIR}/utils/include/
  /usr/lib/i386-linux-gnu/glib-2.0/include
  /usr/lib/x86_64-linux-gnu/glib-2.0/include/
  ${GSTREAMER_gst_INCLUDE_DIR}
  ${GLIB_glib_2_INCLUDE_DIR}
  ${CMAKE_BINARY_DIR}/src/components/
)

if (EXTENDED_MEDIA_MODE)
  include_directories (
    ${GSTREAMER_gst_INCLUDE_DIR}
    ${GLIB_glib_2_INCLUDE_DIR}
  )
endif()

set (SOURCES
 media_manager_impl_test.cc
 main.cc
 media_adapter_impl_test.cc
)

set (LIBRARIES
    MediaManager
    ApplicationManager
    gtest_main
    gmock
    gmock_main
    formatters
    HMI_API
    v4_protocol_v1_2_no_extra
    SmartObjects
    ProtocolHandler
    Utils
    ConfigProfile
    connectionHandler
    MOBILE_API
    jsoncpp
    TransportManager
    HMIMessageHandler
    MessageBroker
    MessageBrokerClient
    MessageBrokerServer
    encryption
    ${RTLIB}
    Resumption
    Policy
    ${SecurityManagerLibrary}
)

if (EXTENDED_MEDIA_MODE)
  list(APPEND LIBRARIES
    ${GSTREAMER_gstreamer_LIBRARY})
endif()

create_test("media_manager_test" "${SOURCES}" "${LIBRARIES}")

if(ENABLE_LOG)
  target_link_libraries("media_manager_test" log4cxx -L${LOG4CXX_LIBS_DIRECTORY})
endif()
# vim: set ts=2 sw=2 et:
