set(UTILS_INCLUDE_DIR ${COMPONENTS_DIR}/utils/include)
set(UTILS_SRC_DIR ${COMPONENTS_DIR}/utils/src)

include_directories (
  ${UTILS_INCLUDE_DIR}
  ${COMPONENTS_DIR}/config_profile/include
  ${COMPONENTS_DIR}/media_manager/include
  ${COMPONENTS_DIR}/protocol_handler/include
  ${LOG4CXX_INCLUDE_DIRECTORY}
)

set (SOURCES
    ${UTILS_SRC_DIR}/bitstream.cc
    ${UTILS_SRC_DIR}/conditional_variable_posix.cc
    ${UTILS_SRC_DIR}/file_system.cc
    ${UTILS_SRC_DIR}/threads/posix_thread.cc
    ${UTILS_SRC_DIR}/threads/thread_manager.cc
    ${UTILS_SRC_DIR}/threads/thread_validator.cc
    ${UTILS_SRC_DIR}/lock_posix.cc
    ${UTILS_SRC_DIR}/rwlock_posix.cc
    ${UTILS_SRC_DIR}/date_time.cc
    ${UTILS_SRC_DIR}/signals_linux.cc
    ${UTILS_SRC_DIR}/system.cc
    ${UTILS_SRC_DIR}/resource_usage.cc
    ${UTILS_SRC_DIR}/appenders_loader.cc
)

if(ENABLE_LOG)
  list(APPEND SOURCES
    ${UTILS_SRC_DIR}/push_log.cc
    ${UTILS_SRC_DIR}/log_message_loop_thread.cc
    ${UTILS_SRC_DIR}/logger_status.cc
  )
endif()

if (BUILD_BACKTRACE_SUPPORT)
    list(APPEND SOURCES
    ${UTILS_SRC_DIR}/back_trace.cc
    )
endif()

if (CMAKE_SYSTEM_NAME STREQUAL "QNX")
    list(APPEND SOURCES
    ${UTILS_SRC_DIR}l/threads/pulse_thread_delegate.cc
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
