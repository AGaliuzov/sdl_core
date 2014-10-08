find_package(Sqlite3 REQUIRED)

include_directories(
  include
  ${GMOCK_INCLUDE_DIRECTORY}
  ${JSONCPP_INCLUDE_DIRECTORY}
  ${CMAKE_SOURCE_DIR}/src/components/policy/src/policy/include/
  ${CMAKE_SOURCE_DIR}/src/components/rpc_base/include
  ${CMAKE_SOURCE_DIR}/src/components/policy/src/policy/sqlite_wrapper/include
  ${CMAKE_SOURCE_DIR}/src/components/policy/src/policy/qdb_wrapper/include
  ${CMAKE_SOURCE_DIR}/src/components/policy/src/policy/usage_statistics/include
  ${CMAKE_SOURCE_DIR}/src/components/utils/include/
)

set(LIBRARIES
  gtest
  gtest_main
  gmock
  gmock_main
  Policy
  ConfigProfile
  ${RTLIB}
  dl
  UsageStatistics
  dbms
  sqlite3
)

set(SOURCES
  src/usage_statistics_test.cc
  src/shared_library_test.cc
  src/generated_code_test.cc
  src/generated_code_with_sqlite_test.cc
  src/policy_manager_impl_test.cc
  src/policy_manager_impl_stress_test.cc
)

if (EXTENDED_POLICY_FLAG)
  add_definitions(-DEXTENDED_POLICY)
  include_directories(${CMAKE_SOURCE_DIR}/src/components/policy/src/policy/policy_table/table_struct_ext)
  list (APPEND SOURCES
    src/sql_pt_ext_representation_test.cc
)
else ()
  include_directories(${CMAKE_SOURCE_DIR}/src/components/policy/src/policy/policy_table/table_struct)
  list (APPEND SOURCES
    src/sql_pt_representation_test.cc
  )
endif ()

if (CMAKE_SYSTEM_NAME STREQUAL "QNX")
  list(REMOVE_ITEM LIBRARIES dl)
  # --- Tests for QDB Wrapper
  include_directories(
    ${CMAKE_SOURCE_DIR}/src/components/policy/src/policy/qdb_wrapper/include
  )
  list (APPEND SOURCES
    src/qdb_wrapper/sql_database_test.cc
    src/qdb_wrapper/sql_query_test.cc
  )
  file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/qdbserver.sh DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
  file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/test-qdb.ini DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
else ()
  # --- Tests for SQLite Wrapper
  include_directories(
    ${CMAKE_SOURCE_DIR}/src/components/policy/src/policy/sqlite_wrapper/include
  )
  list (APPEND SOURCES
    src/sqlite_wrapper/sql_database_test.cc
    src/sqlite_wrapper/sql_query_test.cc
  )
endif()

create_test("test_PolicyManager" "${SOURCES}" "${LIBRARIES}")

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/valid_sdl_pt_update.json DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/log4cxx.properties DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
