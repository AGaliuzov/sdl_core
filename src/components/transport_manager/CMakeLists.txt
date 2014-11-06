set(target TransportManager)
set(TM_SRC_DIR ${COMPONENTS_DIR}/transport_manager/src)

include_directories (
  include
  ${COMPONENTS_DIR}/utils/include/
  ${COMPONENTS_DIR}/protocol_handler/include
  ${COMPONENTS_DIR}/connection_handler/include
  ${COMPONENTS_DIR}/config_profile/include
  ${COMPONENTS_DIR}/resumption/include
  ${JSONCPP_INCLUDE_DIRECTORY}
  ${LIBUSB_INCLUDE_DIRECTORY}
  ${LOG4CXX_INCLUDE_DIRECTORY}
)
if (BUILD_BT_SUPPORT)
  include_directories(
  )
endif()

set (SOURCES
  ${TM_SRC_DIR}/transport_manager_impl.cc
  ${TM_SRC_DIR}/transport_manager_default.cc
  ${TM_SRC_DIR}/transport_adapter/transport_adapter_listener_impl.cc
  ${TM_SRC_DIR}/transport_adapter/transport_adapter_impl.cc
  ${TM_SRC_DIR}/tcp/tcp_transport_adapter.cc
  ${TM_SRC_DIR}/transport_adapter/threaded_socket_connection.cc
  ${TM_SRC_DIR}/tcp/tcp_client_listener.cc
  ${TM_SRC_DIR}/tcp/tcp_device.cc
  ${TM_SRC_DIR}/tcp/tcp_socket_connection.cc
  ${TM_SRC_DIR}/tcp/tcp_connection_factory.cc
)

if (BUILD_AVAHI_SUPPORT)
  list (APPEND SOURCES
  ${TM_SRC_DIR}/tcp/dnssd_service_browser.cc
  )
endif()
if (BUILD_BT_SUPPORT)
  list (APPEND SOURCES
  ${TM_SRC_DIR}/bluetooth/bluetooth_device_scanner.cc
  ${TM_SRC_DIR}/bluetooth/bluetooth_transport_adapter.cc
  ${TM_SRC_DIR}/bluetooth/bluetooth_connection_factory.cc
  ${TM_SRC_DIR}/bluetooth/bluetooth_socket_connection.cc
  ${TM_SRC_DIR}/bluetooth/bluetooth_device.cc
  )
endif()

if (BUILD_USB_SUPPORT)
  list (APPEND SOURCES
    ${TM_SRC_DIR}/usb/usb_aoa_adapter.cc
    ${TM_SRC_DIR}/usb/usb_connection_factory.cc
    ${TM_SRC_DIR}/usb/usb_device_scanner.cc
  )
  if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    list (APPEND SOURCES
    ${TM_SRC_DIR}/usb/libusb/usb_handler.cc
    ${TM_SRC_DIR}/usb/libusb/usb_connection.cc
    ${TM_SRC_DIR}/usb/libusb/platform_usb_device.cc
    )
  elseif(CMAKE_SYSTEM_NAME STREQUAL "QNX")
    list(APPEND SOURCES
    ${TM_SRC_DIR}/usb/qnx/usb_handler.cc
    ${TM_SRC_DIR}/usb/qnx/usb_connection.cc
    ${TM_SRC_DIR}/usb/qnx/platform_usb_device.cc
    )
  endif()
endif(BUILD_USB_SUPPORT)

if (BUILD_MME_SUPPORT)
  list (APPEND SOURCES
    ${TM_SRC_DIR}/mme/mme_transport_adapter.cc
    ${TM_SRC_DIR}/mme/mme_device.cc
    ${TM_SRC_DIR}/mme/iap_device.cc
    ${TM_SRC_DIR}/mme/iap2_device.cc
    ${TM_SRC_DIR}/mme/mme_device_scanner.cc
    ${TM_SRC_DIR}/mme/mme_connection_factory.cc
    ${TM_SRC_DIR}/mme/iap_connection.cc
    ${TM_SRC_DIR}/mme/iap2_connection.cc
    ${TM_SRC_DIR}/mme/protocol_config.cc
  )
endif(BUILD_MME_SUPPORT)

if (CMAKE_SYSTEM_NAME STREQUAL "QNX")
  if (BUILD_AOA_SUPPORT)
    list (APPEND SOURCES
      ${TM_SRC_DIR}/aoa/aoa_wrapper.cc
      ${TM_SRC_DIR}/aoa/aoa_device.cc
      ${TM_SRC_DIR}/aoa/aoa_dynamic_device.cc
      ${TM_SRC_DIR}/aoa/aoa_device_scanner.cc
      ${TM_SRC_DIR}/aoa/aoa_connection.cc
      ${TM_SRC_DIR}/aoa/aoa_connection_factory.cc
      ${TM_SRC_DIR}/aoa/aoa_transport_adapter.cc
    )
  endif(BUILD_AOA_SUPPORT)
endif()

add_library(${target} ${SOURCES})
target_link_libraries(${target} ProtocolLibrary)

if (BUILD_USB_SUPPORT)
  if (CMAKE_SYSTEM_NAME STREQUAL "QNX")
    target_link_libraries(${target} usbdi)
  endif()
endif(BUILD_USB_SUPPORT)

if (BUILD_MME_SUPPORT)
  target_link_libraries (${target} ipod)
  target_link_libraries (${target} iap2client)
  target_link_libraries (${target} qdb)
endif(BUILD_MME_SUPPORT)

if (BUILD_AVAHI_SUPPORT)
  target_link_libraries(${target} avahi-common)
endif()

if(ENABLE_LOG)
  target_link_libraries(${target} log4cxx -L${LOG4CXX_LIBS_DIRECTORY})
endif()

if (BUILD_BT_SUPPORT)
  target_link_libraries(${target} bluetooth)
endif()

if (CMAKE_SYSTEM_NAME STREQUAL "QNX")
  if (BUILD_AOA_SUPPORT)
    target_link_libraries(${target} aoa)
  endif(BUILD_AOA_SUPPORT)
endif()

add_library("transport_manager" ${SOURCES}
        ${TRANSPORT_MANAGER_SOURCES}
)

if(BUILD_TESTS)
  add_subdirectory(test)
endif()