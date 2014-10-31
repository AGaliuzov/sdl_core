include_directories (
  include/
  ${COMPONENTS_DIR}/application_manager/include/
  ${COMPONENTS_DIR}/utils/include/
  ${COMPONENTS_DIR}/smart_objects/include/
  ${COMPONENTS_DIR}/formatters/include/
  ${COMPONENTS_DIR}/config_profile/include/
  ${COMPONENTS_DIR}/media_manager/include/
  ${COMPONENTS_DIR}/protocol_handler/include
  ${JSONCPP_INCLUDE_DIRECTORY}
  ${MESSAGE_BROKER_INCLUDE_DIRECTORY}
  ${COMPONENTS_DIR}
  ${CMAKE_BINARY_DIR}/src/components/
  ${COMPONENTS_DIR}/dbus/include/
  ${CMAKE_SOURCE_DIR}/
  ${LOG4CXX_INCLUDE_DIRECTORY}
)

IF (HMIADAPTER STREQUAL "dbus")
    set (DBUS_SOURCE ./src/dbus_message_adapter.cc)
    set (DBUS_ADAPTER DBus)
ENDIF ()

set (SOURCES
   ${COMPONENTS_DIR}/hmi_message_handler/src/hmi_message_handler_impl.cc
   ${COMPONENTS_DIR}/hmi_message_handler/src/messagebroker_adapter.cc
   ${COMPONENTS_DIR}/hmi_message_handler/src/hmi_message_adapter.cc
   ${COMPONENTS_DIR}/hmi_message_handler/src/mqueue_adapter.cc
   ${DBUS_SOURCE}
)

set (LIBRARIES
  Utils
  ${DBUS_ADAPTER}
  ${RTLIB}
)

add_library("HMIMessageHandler" ${SOURCES})
target_link_libraries("HMIMessageHandler" ${LIBRARIES})

if(ENABLE_LOG)
  target_link_libraries("HMIMessageHandler" log4cxx -L${LOG4CXX_LIBS_DIRECTORY})
endif()
