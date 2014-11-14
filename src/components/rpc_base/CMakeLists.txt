set(RPC_BASE_INCLUDE_DIR ${COMPONENTS_DIR}/rpc_base/include)

include_directories(
  include
  ${JSONCPP_INCLUDE_DIRECTORY}
)

set (SOURCES
  ${COMPONENTS_DIR}/rpc_base/src/rpc_base/rpc_base.cc
)

set (HEADERS
  ${RPC_BASE_INCLUDE_DIR}/rpc_base/gtest_support.h
  ${RPC_BASE_INCLUDE_DIR}/rpc_base/rpc_base_dbus_inl.h
  ${RPC_BASE_INCLUDE_DIR}/rpc_base/rpc_base.h
  ${RPC_BASE_INCLUDE_DIR}/rpc_base/rpc_base_inl.h
  ${RPC_BASE_INCLUDE_DIR}/rpc_base/rpc_base_json_inl.h
  ${RPC_BASE_INCLUDE_DIR}/rpc_base/rpc_message.h
  ${RPC_BASE_INCLUDE_DIR}/rpc_base/validation_report.h
)

add_library(rpc_base ${HEADERS} ${SOURCES})
target_link_libraries(rpc_base jsoncpp)

if(BUILD_TESTS)
  add_subdirectory(test)
endif()