include_directories(
  include/
  ${COMPONENTS_DIR}/protocol_handler/include/
  ${COMPONENTS_DIR}/connection_handler/include
  ${COMPONENTS_DIR}/utils/include/
  ${JSONCPP_INCLUDE_DIRECTORY}
  ${CMAKE_SOURCE_DIR}/src/thirdPartyLibs/jsoncpp/include
  ${APR_INCLUDE_DIRECTORY}
)

set (SOURCES
  ${COMPONENTS_DIR}/security_manager/src/security_manager_impl.cc
  ${COMPONENTS_DIR}/security_manager/src/security_query.cc
  ${COMPONENTS_DIR}/security_manager/src/crypto_manager_impl.cc
  ${COMPONENTS_DIR}/security_manager/src/ssl_context_impl.cc
)

add_library(SecurityManager ${SOURCES})
target_link_libraries(SecurityManager crypto ssl ProtocolHandler jsoncpp ProtocolLibrary)

if(BUILD_TESTS)
  #add_subdirectory(test)
endif()