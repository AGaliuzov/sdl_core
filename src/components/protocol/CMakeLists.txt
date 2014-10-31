include_directories(
  ./include
  ${APR_INCLUDE_DIRECTORY}
)

set(SOURCES
  ${COMPONENTS_DIR}/protocol/src/raw_message.cc
  ${COMPONENTS_DIR}/protocol/src/service_type.cc
  ${COMPONENTS_DIR}/protocol/src/message_priority.cc
  ${COMPONENTS_DIR}/protocol/src/rpc_type.cc
)

add_library(ProtocolLibrary ${SOURCES})
target_link_libraries(ProtocolLibrary Utils)
