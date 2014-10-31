set(SMART_OBJECTS_SRC_DIR ${COMPONENTS_DIR}/smart_objects/src)

include_directories (
  include
  ${COMPONENTS_DIR}/utils/include/
)

set (SOURCES
    ${SMART_OBJECTS_SRC_DIR}/smart_object.cc
    ${SMART_OBJECTS_SRC_DIR}/smart_schema.cc
    ${SMART_OBJECTS_SRC_DIR}/schema_item.cc
    ${SMART_OBJECTS_SRC_DIR}/always_false_schema_item.cc
    ${SMART_OBJECTS_SRC_DIR}/always_true_schema_item.cc
    ${SMART_OBJECTS_SRC_DIR}/default_shema_item.cc
    ${SMART_OBJECTS_SRC_DIR}/bool_schema_item.cc
    ${SMART_OBJECTS_SRC_DIR}/number_schema_item.cc
    ${SMART_OBJECTS_SRC_DIR}/enum_schema_item.cc
    ${SMART_OBJECTS_SRC_DIR}/string_schema_item.cc
    ${SMART_OBJECTS_SRC_DIR}/object_schema_item.cc
    ${SMART_OBJECTS_SRC_DIR}/array_schema_item.cc
)

add_library("SmartObjects" ${SOURCES})

if(ENABLE_LOG)
  target_link_libraries("SmartObjects" log4cxx -L${LOG4CXX_LIBS_DIRECTORY})
endif()

