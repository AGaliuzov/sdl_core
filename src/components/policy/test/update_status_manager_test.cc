/*
 * Copyright (c) 2015, Ford Motor Company
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following
 * disclaimer in the documentation and/or other materials provided with the
 * distribution.
 *
 * Neither the name of the Ford Motor Company nor the names of its contributors
 * may be used to endorse or promote products derived from this software
 * without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <vector>
#include <iostream>
#include <fstream>
#include <string>
#include <ctime>
#include <sstream>
#include <algorithm>

#include "json/reader.h"
#include "json/writer.h"
#include "json/value.h"
#include "gtest/gtest.h"
#include "mock_policy_listener.h"
#include "mock_pt_ext_representation.h"
#include "mock_cache_manager.h"
#include "mock_update_status_manager.h"
#include "policy/policy_manager_impl.h"
#include "policy/cache_manager.h"
#include "policy/update_status_manager.h"
#include "policy/cache_manager_interface.h"
#include "config_profile/profile.h"
#include "sqlite_wrapper/sql_error.h"
#include "sqlite_wrapper/sql_database.h"
#include "sqlite_wrapper/sql_query.h"
#include "table_struct_ext/types.h"
#include "table_struct/types.h"
#include "utils/macro.h"
#include "utils/file_system.h"
#include "utils/date_time.h"
#include "utils/gen_hash.h"
#include "enums.h"

using ::policy::dbms::SQLError;
using ::policy::dbms::SQLDatabase;
using ::policy::dbms::SQLQuery;

using ::testing::_;
using ::testing::Return;
using ::testing::DoAll;
using ::testing::SetArgReferee;
using ::testing::NiceMock;
using ::testing::AtLeast;

using ::policy::PTRepresentation;
using ::policy::MockPolicyListener;
using ::policy::MockPTRepresentation;
using ::policy::MockPTExtRepresentation;
using ::policy::MockCacheManagerInterface;

using ::policy::MockUpdateStatusManager;

using ::policy::PolicyManagerImpl;
using ::policy::PolicyTable;
using ::policy::EndpointUrls;

namespace policy_table = rpc::policy_table_interface_base;

namespace test {
namespace components {
namespace policy {


class UpdateStatusManagerTest : public ::testing::Test {
 protected:
  ::policy::UpdateStatusManager* manager;
  //MockUpdateStatusManager update_manager;
  NiceMock<MockPolicyListener> listener;

  void SetUp() {
    manager = new ::policy::UpdateStatusManager();
    //cache_manager = new MockCacheManagerInterface();
    //manager->set_cache_manager(cache_manager);
    manager->set_listener(&listener);
  }

  void TearDown() {
    delete manager;
  }
};

//enum PolicyTableStatus {
//    StatusUpToDate = 0,
//    StatusUpdatePending,
//    StatusUpdateRequired,
//    StatusUnknown
//};

TEST_F(UpdateStatusManagerTest, OnUpdateSentOut_SetTimeoutExpired_ExpectStatusUpdateNeeded) {
  // Arrange
  manager->OnUpdateSentOut(1);
  ::policy::PolicyTableStatus status = manager->GetLastUpdateStatus();
  EXPECT_EQ(::policy::PolicyTableStatus::StatusUpdatePending, status);
  sleep(2);
  status = manager->GetLastUpdateStatus();
  EXPECT_EQ(::policy::PolicyTableStatus::StatusUpdateRequired, status);
}


}  // namespace policy
}  // namespace components
}  // namespace test

