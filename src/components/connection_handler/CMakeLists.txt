set(ComponentsDir ${CMAKE_SOURCE_DIR}/src/components)
set(CH_SRC_DIR ${ComponentsDir}/connection_handler/src)

include_directories (
  ./include/
  ${ComponentsDir}/protocol_handler/include/
  ${ComponentsDir}/config_profile/include/
  ${ComponentsDir}/utils/include/
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