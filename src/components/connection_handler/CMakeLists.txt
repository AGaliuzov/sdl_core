#set(COMPONENTS_DIR ${CMAKE_SOURCE_DIR}/src/components)
set(CH_SRC_DIR ${COMPONENTS_DIR}/connection_handler/src)

include_directories (
  include
  ${COMPONENTS_DIR}/protocol_handler/include/
  ${COMPONENTS_DIR}/config_profile/include/
  ${COMPONENTS_DIR}/utils/include/
  ${ENCRYPTION_INCLUDE_DIRECTORY}
  ${LOG4CXX_INCLUDE_DIRECTORY}
)

set (SOURCES
   ${CH_SRC_DIR}/connection_handler_impl.cc
   ${CH_SRC_DIR}/connection.cc
   ${CH_SRC_DIR}/device.cc
   ${CH_SRC_DIR}/heartbeat_monitor.cc
)

set(LIBRARIES
  ConfigProfile
  ProtocolLibrary
)

add_library(connectionHandler ${SOURCES})
target_link_libraries(connectionHandler encryption)

if(BUILD_TESTS)
  add_subdirectory(test)
endif()