include_directories (
  ${LOG4CXX_INCLUDE_DIRECTORY}
  ${GMOCK_INCLUDE_DIRECTORY}
  ${COMPONENTS_DIR}/smart_objects/include
  ${COMPONENTS_DIR}/transport_manager/include
  ${COMPONENTS_DIR}/utils/include
  ${JSONCPP_INCLUDE_DIRECTORY}
  ${CMAKE_BINARY_DIR}
)

set (LIBRARIES
    gmock
    gmock_main
    gtest
    gtest_main  
    ProtocolHandler
    SmartObjects
    transport_manager
    jsoncpp
)

set(SOURCES
${COMPONENTS_DIR}/transport_manager/test/main.cc  
${COMPONENTS_DIR}/transport_manager/test/mock_application.cc
${COMPONENTS_DIR}/transport_manager/test/mock_connection_factory.cc  ##MockConnectionFactory
${COMPONENTS_DIR}/transport_manager/test/mock_connection.cc       #ok
${COMPONENTS_DIR}/transport_manager/test/mock_device.cc            #ok
${COMPONENTS_DIR}/transport_manager/test/mock_device_scanner.cc     ##ok
${COMPONENTS_DIR}/transport_manager/test/raw_message_matcher.cc    #ok
#${COMPONENTS_DIR}/transport_manager/test/test_dnssd_service_browser.cc
${COMPONENTS_DIR}/transport_manager/test/test_tcp_transport_adapter.cc ##dnssd_service_browser.cc.o
#${COMPONENTS_DIR}/transport_manager/test/transport_manager_instance_test.cc    ##should be changed
#${COMPONENTS_DIR}/transport_manager/test/mock_transport_adapter.cc         ##problem in transport_manager::transport_adapter::TransportAdapterImpl::Init
#${COMPONENTS_DIR}/transport_manager/test/transport_manager_instance_test.h              
#${COMPONENTS_DIR}/transport_manager/test/mock_transport_adapter_listener.cc  
#${COMPONENTS_DIR}/transport_manager/test/transport_manager_test.cc
)
#${COMPONENTS_DIR}/transport_manager/test/test_dnssd_service_browser.cc
#${COMPONENTS_DIR}/transport_manager/test/test_tcp_transport_adapter.cc
#${COMPONENTS_DIR}/transport_manager/test/mock_device_scanner.cc 
#${COMPONENTS_DIR}/transport_manager/test/transport_manager_instance_test.cc   
#${COMPONENTS_DIR}/transport_manager/test/mock_transport_adapter.cc           
#${COMPONENTS_DIR}/transport_manager/test/transport_manager_instance_test.h              
#${COMPONENTS_DIR}/transport_manager/test/transport_manager_test.cc

create_test("test_transport_manager" "${SOURCES}" "${LIBRARIES}")


