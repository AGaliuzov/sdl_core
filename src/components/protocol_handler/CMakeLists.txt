include_directories(
  include
  ${COMPONENTS_DIR}/utils/include/
  ${COMPONENTS_DIR}/connection_handler/include/
  ${COMPONENTS_DIR}/config_profile/include/
  ${LOG4CXX_INCLUDE_DIRECTORY}
)

set(SOURCES
    ./src/incoming_data_handler.cc
    ./src/protocol_handler_impl.cc
    ./src/protocol_packet.cc
    ./src/protocol_payload.cc
    ${COMPONENTS_DIR}/protocol_handler/src/protocol_handler_impl.cc
    ${COMPONENTS_DIR}/protocol_handler/src/protocol_packet.cc
    ${COMPONENTS_DIR}/protocol_handler/src/protocol_payload.cc
)

set(LIBRARIES
  ProtocolLibrary
  Utils
)

add_library(ProtocolHandler ${SOURCES})
target_link_libraries(ProtocolHandler ${LIBRARIES})
