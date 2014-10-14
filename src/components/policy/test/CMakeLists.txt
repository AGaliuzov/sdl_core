

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

set(testLibraries
  gtest
  gmock
  Utils
  Policy
  ConfigProfile
  ${RTLIB}
  dl
  UsageStatistics
  dbms
)

set(testSources
  main.cc
  usage_statistics_test.cc
  shared_library_test.cc
  generated_code_test.cc
  policy_manager_impl_test.cc
)

if (EXTENDED_POLICY_FLAG)
  add_definitions(-DEXTENDED_POLICY)
  include_directories(${CMAKE_SOURCE_DIR}/src/components/policy/src/policy/policy_table/table_struct_ext)
  list (APPEND testSources
    sql_pt_ext_representation_test.cc
)
else ()
  include_directories(${CMAKE_SOURCE_DIR}/src/components/policy/src/policy/policy_table/table_struct)
  list (APPEND testSources
    sql_pt_representation_test.cc
  )
endif ()

if (CMAKE_SYSTEM_NAME STREQUAL "QNX")
  list(REMOVE_ITEM testLibraries dl)
  # --- Tests for QDB Wrapper
  include_directories(
    ${CMAKE_SOURCE_DIR}/src/components/policy/src/policy/qdb_wrapper/include
  )
  list (APPEND testSources
    qdb_wrapper/sql_database_test.cc
    qdb_wrapper/sql_query_test.cc
  )
  file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/qdbserver.sh DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
  file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/test-qdb.ini DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
  file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/policy.sql DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
else ()
  # --- Tests for SQLite Wrapper
  find_package(Sqlite3 REQUIRED)
  include_directories(
    ${CMAKE_SOURCE_DIR}/src/components/policy/src/policy/sqlite_wrapper/include
  )
  list (APPEND testSources
    sqlite_wrapper/sql_database_test.cc
    sqlite_wrapper/sql_query_test.cc
    generated_code_with_sqlite_test.cc
    policy_manager_impl_stress_test.cc
  )
  list (APPEND testLibraries sqlite3)
endif()

add_executable(policy_test ${testSources})
target_link_libraries(policy_test ${testLibraries})

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/valid_sdl_pt_update.json DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/sdl_preloaded_pt.json DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/log4cxx.properties DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
