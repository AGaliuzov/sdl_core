include_directories(
  include
  ${COMPONENTS_DIR}/utils/include/
  ${COMPONENTS_DIR}/protocol_handler/include/
  ${COMPONENTS_DIR}/connection_handler/include/
  ${COMPONENTS_DIR}/config_profile/include/
  ${LOG4CXX_INCLUDE_DIRECTORY}
)

set(SOURCES
    ${COMPONENTS_DIR}/protocol_handler/src/incoming_data_handler.cc
    ${COMPONENTS_DIR}/protocol_handler/src/protocol_handler_impl.cc
    ${COMPONENTS_DIR}/protocol_handler/src/protocol_packet.cc
    ${COMPONENTS_DIR}/protocol_handler/src/protocol_payload.cc
)

set(LIBRARIES
  ProtocolLibrary
  Utils
)

get_property(dirs DIRECTORY "" PROPERTY LIBRARIES)
foreach(dir ${dirs})
  message(STATUS "libraries='${dir}'")
endforeach()


add_library(ProtocolHandler ${SOURCES})
target_link_libraries(ProtocolHandler ${LIBRARIES})
