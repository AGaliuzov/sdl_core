set(UtilsIncludeDir ${CMAKE_SOURCE_DIR}/src/components/utils/include)
set(UtilsSrcDir ${CMAKE_SOURCE_DIR}/src/components/utils/src)
set(ComponentsDir ${CMAKE_SOURCE_DIR}/src/components)


include_directories (
  ${UtilsIncludeDir}
  ${ComponentsDir}/config_profile/include
  ${ComponentsDir}/media_manager/include
  ${ComponentsDir}/protocol_handler/include
  ${LOG4CXX_INCLUDE_DIRECTORY}
)

set (SOURCES
    ${UtilsSrcDir}/bitstream.cc
    ${UtilsSrcDir}/conditional_variable_posix.cc
    ${UtilsSrcDir}/file_system.cc
    ${UtilsSrcDir}/threads/posix_thread.cc
    ${UtilsSrcDir}/threads/thread_manager.cc
    ${UtilsSrcDir}/threads/thread_validator.cc
    ${UtilsSrcDir}/lock_posix.cc
    ${UtilsSrcDir}/rwlock_posix.cc
    ${UtilsSrcDir}/date_time.cc
    ${UtilsSrcDir}/signals_linux.cc
    ${UtilsSrcDir}/system.cc
    ${UtilsSrcDir}/resource_usage.cc
    ${UtilsSrcDir}/appenders_loader.cc
)

if(ENABLE_LOG)
  list(APPEND SOURCES
    ${UtilsSrcDir}/push_log.cc
    ${UtilsSrcDir}/log_message_loop_thread.cc
    ${UtilsSrcDir}/logger_status.cc
  )
endif()

if (BUILD_BACKTRACE_SUPPORT)
    list(APPEND SOURCES
    ${UtilsSrcDir}/back_trace.cc
    )
endif()

if (CMAKE_SYSTEM_NAME STREQUAL "QNX")
    list(APPEND SOURCES
    ${UtilsSrcDir}l/threads/pulse_thread_delegate.cc
    )
endif()


add_library("Utils" SHARED ${SOURCES})

list(APPEND LIBRARIES dl)

if(ENABLE_LOG)
  list(APPEND LIBRARIES log4cxx -L${LOG4CXX_LIBS_DIRECTORY})
  list(APPEND LIBRARIES apr-1 -L${APR_LIBS_DIRECTORY})
  list(APPEND LIBRARIES aprutil-1 -L${APR_UTIL_LIBS_DIRECTORY})
  ADD_DEPENDENCIES(Utils install-3rd_party_logger)
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  target_link_libraries("Utils" pthread ${RTLIB})
endif()

target_link_libraries("Utils" ${LIBRARIES})

if(BUILD_TESTS)
  add_subdirectory(test)
endif()

install(TARGETS "Utils"
  DESTINATION bin
  PERMISSIONS
    OWNER_READ OWNER_WRITE
    GROUP_READ
    WORLD_READ
)
