include_directories(
  ${GMOCK_INCLUDE_DIRECTORY}
  ${LOG4CXX_INCLUDE_DIRECTORY}
  ${CMAKE_SOURCE_DIR}/src/components/utils/include
  ./include
  ${CMAKE_SOURCE_DIR}/src/components/protocol_handler/include
  ${CMAKE_SOURCE_DIR}/src/components/protocol_handler/test/include
  ${CMAKE_SOURCE_DIR}/src/components/security_manager/test/include
  ${CMAKE_SOURCE_DIR}/src/components/transport_manager/test/include
  ${SecurityManagerIncludeDir}
  ${CMAKE_SOURCE_DIR}/src/components/config_profile/include
)

set(LIBRARIES
  gtest
  gtest_main
  gmock
  gmock_main
  ProtocolHandler
  connectionHandler
  Utils
  ConfigProfile
  ${RTLIB}
  ProtocolLibrary
)

set(SOURCES
  protocol_handler_tm_test.cc
  incoming_data_handler_test.cc
  protocol_header_validator_test.cc
)

#create_test(test_protocol_handler "${SOURCES}" "${LIBRARIES}")
#add_library(test_protocol_handler_test ${SOURCES})
