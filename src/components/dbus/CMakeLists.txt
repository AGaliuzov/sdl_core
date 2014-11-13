include_directories (
  ${DBUS_INCLUDE_DIRS}
  ${CMAKE_CURRENT_SOURCE_DIR}/include/
  ${CMAKE_SOURCE_DIR}/
  ${COMPONENTS_DIR}
  ${COMPONENTS_DIR}/formatters/include/
  ${COMPONENTS_DIR}/smart_objects/include/
  ${COMPONENTS_DIR}/utils/include/
  ${CMAKE_BINARY_DIR}/src/components/
)

set (SOURCES
  ${CMAKE_CURRENT_BINARY_DIR}/introspection_xml.cc
  ${CMAKE_CURRENT_BINARY_DIR}/message_descriptions.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/src/dbus_adapter.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/src/dbus_message.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/src/dbus_message_controller.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/src/schema.cc
)

set (LIBRARIES dbus-1 -L${DBUS_LIBS_DIRECTORY})


add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/message_descriptions.cc
  COMMAND python ${CMAKE_CURRENT_SOURCE_DIR}/codegen/make_message_descriptions.py
  --infile ${CMAKE_CURRENT_SOURCE_DIR}/../interfaces/QT_HMI_API.xml
  --outdir ${CMAKE_CURRENT_BINARY_DIR}
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/../interfaces/QT_HMI_API.xml
    ${CMAKE_CURRENT_SOURCE_DIR}/codegen/make_message_descriptions.py
)

add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/introspection_xml.cc
  COMMAND python ${CMAKE_CURRENT_SOURCE_DIR}/codegen/make_introspection_c.py
  --infile ${CMAKE_CURRENT_SOURCE_DIR}/../interfaces/QT_HMI_API.xml
  --outdir ${CMAKE_CURRENT_BINARY_DIR}
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/../interfaces/QT_HMI_API.xml
    ${CMAKE_CURRENT_SOURCE_DIR}/codegen/make_introspection_c.py
)

add_library("DBus" ${SOURCES})
ADD_DEPENDENCIES("DBus" Utils install-3rd_party_dbus)

target_link_libraries("DBus" "${LIBRARIES}")
