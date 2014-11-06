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
    SmartObjects
    transport_manager
    jsoncpp
)

set(SOURCES
${COMPONENTS_DIR}/transport_manager/test/main.cc  
${COMPONENTS_DIR}/transport_manager/test/mock_application.cc
#${COMPONENTS_DIR}/transport_manager/test/TransportManagerTestWithCorrectDeviceAdapter.cpp
#${COMPONENTS_DIR}/transport_manager/test/mock_connection_factory.cc  
#${COMPONENTS_DIR}/transport_manager/test/mock_connection.cc   
#${COMPONENTS_DIR}/transport_manager/test/mock_device.cc
#${COMPONENTS_DIR}/transport_manager/test/TcpSimulator.cpp
#${COMPONENTS_DIR}/transport_manager/test/TransportManagerTestWithHighDataTransferRate.cpp
${COMPONENTS_DIR}/transport_manager/test/raw_message_matcher.cc 
)
#${COMPONENTS_DIR}/transport_manager/test/test_dnssd_service_browser.cc
#${COMPONENTS_DIR}/transport_manager/test/TransportManagerTestWithIncorrectDeviceAdapter.cpp
#${COMPONENTS_DIR}/transport_manager/test/HandleGeneratorsTest.cpp
#${COMPONENTS_DIR}/transport_manager/test/test_tcp_transport_adapter.cc
#${COMPONENTS_DIR}/transport_manager/test/TransportManagerTestWithMultipleClients.cpp
#${COMPONENTS_DIR}/transport_manager/test/mock_device_scanner.cc 
#${COMPONENTS_DIR}/transport_manager/test/transport_manager_instance_test.cc  
#${COMPONENTS_DIR}/transport_manager/test/TransportManagerTestWithMultipleDeviceAdapters.cpp
#${COMPONENTS_DIR}/transport_manager/test/ListenersTest.cpp         
#${COMPONENTS_DIR}/transport_manager/test/mock_transport_adapter.cc           
#${COMPONENTS_DIR}/transport_manager/test/transport_manager_instance_test.h   
#${COMPONENTS_DIR}/transport_manager/test/TransportManagerTestWithMultiThreadedClient.cpp                 
#${COMPONENTS_DIR}/transport_manager/test/mock_transport_adapter_listener.cc  
#${COMPONENTS_DIR}/transport_manager/test/transport_manager_test.cc

create_test("test_transport_manager" "${SOURCES}" "${LIBRARIES}")


