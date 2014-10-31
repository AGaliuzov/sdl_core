set(FORMATTERS_SRC_DIR  ${COMPONENTS_DIR}/formatters/src)

include_directories (
  include/
  ${COMPONENTS_DIR}/protocol_handler/include/
  ${COMPONENTS_DIR}/utils/include/
  ${JSONCPP_INCLUDE_DIRECTORY}
  ${MESSAGE_BROKER_INCLUDE_DIRECTORY}
  ${COMPONENTS_DIR}/smart_objects/include
)

set (SOURCES
    ${FORMATTERS_SRC_DIR}/CSmartFactory.cpp
)

set (FORMATTER_SOURCES
   ${FORMATTERS_SRC_DIR}/CFormatterJsonBase.cpp
   ${FORMATTERS_SRC_DIR}/CFormatterJsonSDLRPCv1.cpp
   ${FORMATTERS_SRC_DIR}/CFormatterJsonSDLRPCv2.cpp
   ${FORMATTERS_SRC_DIR}/formatter_json_rpc.cc
   ${FORMATTERS_SRC_DIR}/meta_formatter.cc
   ${FORMATTERS_SRC_DIR}/generic_json_formatter.cc
)

add_library("formatters" ${SOURCES}
        ${FORMATTER_SOURCES}
)

if(BUILD_TESTS)
  add_subdirectory(test)
endif()
