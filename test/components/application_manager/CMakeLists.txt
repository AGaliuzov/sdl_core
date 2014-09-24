############################## Subfolders #####################################
# --- Policies table test
#add_subdirectory(./policies_manager)

###############################################################################

include_directories (
  ${GMOCK_INCLUDE_DIRECTORY}
  ../../../src/components/application_manager/include
  ../../../src/components/smart_objects/include
  ../../../src/components/formatters/include/
  ../../../src/components/utils/include/
  ${JSONCPP_INCLUDE_DIRECTORY}
  ${CMAKE_BINARY_DIR}/src/components/
)

set(LIBRARIES
    gtest
    gtest_main
    gmock
    gmock_main
    ApplicationManager
    formatters
    jsoncpp
    MOBILE_API
    HMI_API
    SmartObjects
    ProtocolLibrary
)

if(ENABLE_LOG)
  list(APPEND LIBRARIES log4cxx -L${LOG4CXX_LIBS_DIRECTORY})
  list(APPEND LIBRARIES apr-1 -L${APR_LIBS_DIRECTORY})
  list(APPEND LIBRARIES aprutil-1 -L${APR_UTIL_LIBS_DIRECTORY})
  list(APPEND LIBRARIES expat -L${EXPAT_LIBS_DIRECTORY})
endif()

#create_test("test_APIVersionConverterV1Test" "./api_converter_v1_test.cpp" "${LIBRARIES}")
create_test("test_formatters_commands" "./formatters_commands.cc" "${LIBRARIES}")
#create_test("test_schema_factory_test" "./schema_factory_test.cc" "${LIBRARIES}")
add_library("test_FormattersCommandsTest" "./formatters_commands.cc")
