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

#include "gtest/gtest.h"
#include "mock_policy_listener.h"
#include "policy/policy_manager_impl.h"
#include "policy/update_status_manager.h"

using ::policy::MockPolicyListener;

namespace test {
namespace components {
namespace policy {

class UpdateStatusManagerTest : public ::testing::Test {
  protected:
    ::policy::UpdateStatusManager* manager;
    ::policy::PolicyTableStatus status;

    void SetUp() {
      manager = new ::policy::UpdateStatusManager();
    }

    void TearDown() {
      delete manager;
    }
};

TEST_F(UpdateStatusManagerTest,
       OnUpdateSentOut_WaitForTimeoutExpired_ExpectStatusUpdateNeeded) {
  // Arrange
  manager->OnUpdateSentOut(1);
  status = manager->GetLastUpdateStatus();
  EXPECT_EQ(::policy::PolicyTableStatus::StatusUpdatePending, status);
  sleep(2);
  status = manager->GetLastUpdateStatus();
  EXPECT_EQ(::policy::PolicyTableStatus::StatusUpdateRequired, status);
}

TEST_F(UpdateStatusManagerTest,
       OnUpdateTimeOutOccurs_SetTimeoutExpired_ExpectStatusUpdateNeeded) {
  // Arrange
  status = manager->GetLastUpdateStatus();
  EXPECT_EQ(::policy::PolicyTableStatus::StatusUpToDate, status);
  manager->OnUpdateTimeoutOccurs();
  status = manager->GetLastUpdateStatus();
  EXPECT_EQ(::policy::PolicyTableStatus::StatusUpdateRequired, status);
}

TEST_F(UpdateStatusManagerTest,
       OnValidUpdateReceived_SetValidUpdateReceived_ExpectStatusUpToDate) {
  // Arrange
  manager->OnUpdateSentOut(1);
  status = manager->GetLastUpdateStatus();
  EXPECT_EQ(::policy::PolicyTableStatus::StatusUpdatePending, status);
  manager->OnValidUpdateReceived();
  status = manager->GetLastUpdateStatus();
  EXPECT_EQ(::policy::PolicyTableStatus::StatusUpToDate, status);
}

TEST_F(UpdateStatusManagerTest,
       OnWrongUpdateReceived_SetWrongUpdateReceived_ExpectStatusUpdateNeeded) {
  // Arrange
  manager->OnUpdateSentOut(1);
  status = manager->GetLastUpdateStatus();
  EXPECT_EQ(::policy::PolicyTableStatus::StatusUpdatePending, status);
  manager->OnWrongUpdateReceived();
  status = manager->GetLastUpdateStatus();
  EXPECT_EQ(::policy::PolicyTableStatus::StatusUpdateRequired, status);
}

TEST_F(UpdateStatusManagerTest,
       OnResetDefaulPT_ResetPTtoDefaultState_ExpectPTinDefaultState) {
  // Arrange
  manager->OnUpdateSentOut(1);
  status = manager->GetLastUpdateStatus();
  EXPECT_EQ(::policy::PolicyTableStatus::StatusUpdatePending, status);
  // Reset PT to default state with flag update required
  manager->OnResetDefaultPT(true);
  status = manager->GetLastUpdateStatus();
  EXPECT_EQ(::policy::PolicyTableStatus::StatusUpdateRequired, status);
}

TEST_F(UpdateStatusManagerTest,
       OnResetDefaulPT2_ResetPTtoDefaultState_ExpectPTinDefaultState) {
  // Arrange
  manager->OnUpdateSentOut(1);
  status = manager->GetLastUpdateStatus();
  EXPECT_EQ(::policy::PolicyTableStatus::StatusUpdatePending, status);
  // Reset PT to default state with flag update not needed
  manager->OnResetDefaultPT(false);
  status = manager->GetLastUpdateStatus();
  EXPECT_EQ(::policy::PolicyTableStatus::StatusUpToDate, status);
}

TEST_F(UpdateStatusManagerTest, OnResetRetrySequence_ExpectStatusUpToDate) {
  // Arrange
  manager->OnUpdateSentOut(1);
  status = manager->GetLastUpdateStatus();
  EXPECT_EQ(::policy::PolicyTableStatus::StatusUpdatePending, status);
  manager->OnResetRetrySequence();
  status = manager->GetLastUpdateStatus();
  EXPECT_EQ(::policy::PolicyTableStatus::StatusUpdatePending, status);
}

TEST_F(UpdateStatusManagerTest,
       OnNewApplicationAdded_ExpectStatusUpdateNeeded) {
  // Arrange
  manager->OnUpdateSentOut(1);
  status = manager->GetLastUpdateStatus();
  EXPECT_EQ(::policy::PolicyTableStatus::StatusUpdatePending, status);
  manager->OnNewApplicationAdded();
  status = manager->GetLastUpdateStatus();
  EXPECT_EQ(::policy::PolicyTableStatus::StatusUpdatePending, status);
  EXPECT_TRUE(manager->IsUpdatePending());
  EXPECT_TRUE(manager->IsUpdateRequired());
}

TEST_F(UpdateStatusManagerTest, ScheduleUpdate_ExpectStatusUpdateNeeded) {
  // Arrange
  manager->OnUpdateSentOut(1);
  status = manager->GetLastUpdateStatus();
  EXPECT_EQ(::policy::PolicyTableStatus::StatusUpdatePending, status);
  manager->OnValidUpdateReceived();
  status = manager->GetLastUpdateStatus();
  EXPECT_EQ(::policy::PolicyTableStatus::StatusUpToDate, status);
  EXPECT_FALSE(manager->IsUpdatePending());
  EXPECT_FALSE(manager->IsUpdateRequired());
  manager->ScheduleUpdate();
  status = manager->GetLastUpdateStatus();
  EXPECT_EQ(::policy::PolicyTableStatus::StatusUpdateRequired, status);
  EXPECT_FALSE(manager->IsUpdatePending());
  EXPECT_TRUE(manager->IsUpdateRequired());
}

TEST_F(UpdateStatusManagerTest,
       ResetUpdateSchedule_SetUpdateScheduleThenReset_ExpectStatusUpToDate) {
  // Arrange
  status = manager->GetLastUpdateStatus();
  EXPECT_EQ(::policy::PolicyTableStatus::StatusUpToDate, status);
  EXPECT_FALSE(manager->IsUpdatePending());
  EXPECT_FALSE(manager->IsUpdateRequired());
  manager->ScheduleUpdate();
  EXPECT_TRUE(manager->IsUpdateRequired());
  manager->OnPolicyInit(false);
  EXPECT_TRUE(manager->IsUpdateRequired());
  manager->ResetUpdateSchedule();
  EXPECT_FALSE(manager->IsUpdateRequired());
  status = manager->GetLastUpdateStatus();
  EXPECT_EQ(::policy::PolicyTableStatus::StatusUpToDate, status);
}

TEST_F(UpdateStatusManagerTest,
       OnPolicyInit_SetUpdateRequired_ExpectStatusUpdateNeeded) {
  // Arrange
  manager->OnPolicyInit(true);
  status = manager->GetLastUpdateStatus();
  EXPECT_EQ(::policy::PolicyTableStatus::StatusUpdateRequired, status);
  EXPECT_FALSE(manager->IsUpdatePending());
  EXPECT_TRUE(manager->IsUpdateRequired());
}

TEST_F(UpdateStatusManagerTest,
       OnPolicyInit_SetUpdateNotRequired_ExpectStatusUpToDate) {
  // Arrange
  manager->OnPolicyInit(false);
  status = manager->GetLastUpdateStatus();
  EXPECT_EQ(::policy::PolicyTableStatus::StatusUpToDate, status);
  EXPECT_FALSE(manager->IsUpdatePending());
  EXPECT_FALSE(manager->IsUpdateRequired());
}

TEST_F(UpdateStatusManagerTest,
       StringifiedUpdateStatus_SetStatuses_ExpectCorrectStringifiedStatuses) {
  // Arrange
  manager->OnPolicyInit(false);
  // Check
  EXPECT_EQ("UP_TO_DATE", manager->StringifiedUpdateStatus());
  manager->OnPolicyInit(true);
  // Check
  EXPECT_EQ("UPDATE_NEEDED", manager->StringifiedUpdateStatus());
  manager->OnUpdateSentOut(1);
  // Check
  EXPECT_EQ("UPDATING", manager->StringifiedUpdateStatus());
}

TEST_F(UpdateStatusManagerTest,
       OnAppSearchStartedCompleted_ExpectAppSearchCorrectStatus) {
  // Arrange
  manager->OnAppsSearchStarted();
  // Check
  EXPECT_TRUE(manager->IsAppsSearchInProgress());
  // Arrange
  manager->OnAppsSearchCompleted();
  // Check
  EXPECT_FALSE(manager->IsAppsSearchInProgress());
}

}  // namespace policy
}  // namespace components
}  // namespace test
