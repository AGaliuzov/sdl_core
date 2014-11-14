include_directories (
  include
  ${COMPONENTS_DIR}/utils/include/
  ${COMPONENTS_DIR}/config_profile/include/
  ${JSONCPP_INCLUDE_DIRECTORY}
  ${LOG4CXX_INCLUDE_DIRECTORY}
)

set (SOURCES
  ${COMPONENTS_DIR}/resumption/src/last_state.cc
)

add_library("Resumption" ${SOURCES})
