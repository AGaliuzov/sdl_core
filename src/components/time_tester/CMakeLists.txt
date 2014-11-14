set(TIME_TESTER_SRC_DIR ${COMPONENTS_DIR}/time_tester/src)

include_directories (
  include/time_tester
  ${COMPONENTS_DIR}/utils/include/
  ${COMPONENTS_DIR}/protocol_handler/include/
  ${COMPONENTS_DIR}/connection_handler/include/
  ${COMPONENTS_DIR}/transport_manager/include/
  ${COMPONENTS_DIR}/application_manager/include/
  ${COMPONENTS_DIR}/hmi_message_handler/include/
  ${COMPONENTS_DIR}/formatters/include/
  ${COMPONENTS_DIR}/media_manager/include/
  ${COMPONENTS_DIR}/smart_objects/include/
  ${COMPONENTS_DIR}/config_profile/include/
  ${JSONCPP_INCLUDE_DIRECTORY}
  ${CMAKE_BINARY_DIR}/src/components/
  ${LOG4CXX_INCLUDE_DIRECTORY}
)

set (SOURCES
    ${TIME_TESTER_SRC_DIR}/metric_wrapper.cc
    ${TIME_TESTER_SRC_DIR}/time_manager.cc
    ${TIME_TESTER_SRC_DIR}/application_manager_observer.cc
    ${TIME_TESTER_SRC_DIR}/transport_manager_observer.cc
    ${TIME_TESTER_SRC_DIR}/protocol_handler_observer.cc
    ${TIME_TESTER_SRC_DIR}/application_manager_metric.cc
    ${TIME_TESTER_SRC_DIR}/transport_manager_metric.cc
    ${TIME_TESTER_SRC_DIR}/protocol_handler_metric.cc
)

add_library("TimeTester" ${SOURCES})
target_link_libraries("TimeTester" ${LIBRARIES})
