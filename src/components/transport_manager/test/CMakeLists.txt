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
    gtest
    gmock_main
    gtest_main
    ConfigProfile
 
    transport_manager
    ApplicationManagerTest 
#    SmartObjects        
#    ProtocolLibrary
#    ProtocolHandler
#    connectionHandler
#    HMIMessageHandler
    Utils
    
    ConfigProfile
    Resumption
    SmartObjects
    jsoncpp 
    
)

if (BUILD_AVAHI_SUPPORT)
  list(APPEND LIBRARIES avahi-client avahi-common)
endif()


set(SOURCES
${COMPONENTS_DIR}/transport_manager/test/main.cc  
${COMPONENTS_DIR}/transport_manager/test/mock_application.cc
${COMPONENTS_DIR}/transport_manager/test/mock_connection_factory.cc  ##ok
${COMPONENTS_DIR}/transport_manager/test/mock_connection.cc       #ok
${COMPONENTS_DIR}/transport_manager/test/mock_device.cc            #ok
${COMPONENTS_DIR}/transport_manager/test/mock_device_scanner.cc     ##ok
${COMPONENTS_DIR}/transport_manager/test/raw_message_matcher.cc    #ok
${COMPONENTS_DIR}/transport_manager/test/mock_transport_adapter_listener.cc   #ok
${COMPONENTS_DIR}/transport_manager/test/test_dnssd_service_browser.cc     #ok
${COMPONENTS_DIR}/transport_manager/test/test_tcp_transport_adapter.cc     ##ok
#${COMPONENTS_DIR}/transport_manager/test/transport_manager_instance_test.cc    ##use outdated tests
${COMPONENTS_DIR}/transport_manager/test/mock_transport_adapter.cc         #ok
${COMPONENTS_DIR}/transport_manager/test/transport_manager_test.cc    #ok
)          

create_test("test_transport_manager" "${SOURCES}" "${LIBRARIES}")


