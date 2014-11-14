include_directories (
  include
  ${COMPONENTS_DIR}/utils/include/
  ${LOG4CXX_INCLUDE_DIRECTORY}
)

set (SOURCES
    ${COMPONENTS_DIR}/config_profile/src/profile.cc
    ${COMPONENTS_DIR}/config_profile/src/ini_file.cc
)

add_library("ConfigProfile" ${SOURCES})
target_link_libraries("ConfigProfile" Utils)
