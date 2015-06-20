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

#include <gtest/gtest.h>
#include "application_manager/hmi_state.h"
#include "application_manager/state_controller.h"
#include "application_manager/usage_statistics.h"
#include "application_manager_mock.h"
#include "application_mock.h"
#include "statistics_manager_mock.h"
#include "state_context_mock.h"
#include "utils/lock.h"
#include "utils/data_accessor.h"
#include "utils/make_shared.h"
#include "application_manager/message_helper.h"
#include "application_manager/event_engine/event.h"
#include "application_manager/smart_object_keys.h"

namespace am = application_manager;
using ::testing::_;
using ::testing::Return;
using ::testing::ReturnRef;
using ::testing::ReturnPointee;
using ::testing::Mock;
using ::testing::NiceMock;
using ::testing::InSequence;
using ::testing::Truly;

class MessageHelperMock {
 public:
  MOCK_METHOD3(SendActivateAppToHMI,
               uint32_t(uint32_t const app_id,
                        hmi_apis::Common_HMILevel::eType level,
                        bool send_policy_priority));
};

static MessageHelperMock* message_helper_mock_;

uint32_t application_manager::MessageHelper::SendActivateAppToHMI(
    uint32_t const app_id, hmi_apis::Common_HMILevel::eType level,
    bool send_policy_priority) {
  return message_helper_mock_->SendActivateAppToHMI(app_id, level,
                                                    send_policy_priority);
}

namespace state_controller_test {

struct HmiStatesComparator {
  mobile_apis::HMILevel::eType hmi_level_;
  mobile_apis::AudioStreamingState::eType audio_streaming_state_;
  mobile_apis::SystemContext::eType system_context_;

  HmiStatesComparator(
      mobile_apis::HMILevel::eType hmi_level,
      mobile_apis::AudioStreamingState::eType audio_streaming_state,
      mobile_apis::SystemContext::eType system_context)
      : hmi_level_(hmi_level),
        audio_streaming_state_(audio_streaming_state),
        system_context_(system_context) {}

  HmiStatesComparator(am::HmiStatePtr state_ptr)
      : hmi_level_(state_ptr->hmi_level()),
        audio_streaming_state_(state_ptr->audio_streaming_state()),
        system_context_(state_ptr->system_context()) {}

  bool operator()(am::HmiStatePtr state_ptr) const {
    return state_ptr->hmi_level() == hmi_level_ &&
           state_ptr->audio_streaming_state() == audio_streaming_state_ &&
           state_ptr->system_context() == system_context_;
  }
};

#define MEDIA true
#define NOT_MEDIA false
#define VC true
#define NOT_VC false
#define NAVI true
#define NOT_NAVI false

class StateControllerTest : public ::testing::Test {
 public:
  StateControllerTest()
      : ::testing::Test(),
        usage_stat("0", utils::SharedPtr<us::StatisticsManager>(
                            new StatisticsManagerMock)),
        applications_(application_set_, applications_lock_),
        state_ctrl_(&app_manager_mock_) {}
  NiceMock<ApplicationManagerMock> app_manager_mock_;

  am::UsageStatistics usage_stat;

  am::ApplicationSet application_set_;
  mutable sync_primitives::Lock applications_lock_;
  DataAccessor<am::ApplicationSet> applications_;
  am::StateController state_ctrl_;

  am::ApplicationSharedPtr simple_app_;
  NiceMock<ApplicationMock>* simple_app_ptr_;
  uint32_t simple_app_id_ = 1721;

  am::ApplicationSharedPtr navi_app_;
  NiceMock<ApplicationMock>* navi_app_ptr_;
  uint32_t navi_app_id_ = 1762;

  am::ApplicationSharedPtr media_app_;
  NiceMock<ApplicationMock>* media_app_ptr_;
  uint32_t media_app_id_ = 1801;

  am::ApplicationSharedPtr vc_app_;
  NiceMock<ApplicationMock>* vc_app_ptr_;
  uint32_t vc_app_id_ = 1825;

  am::ApplicationSharedPtr media_navi_app_;
  NiceMock<ApplicationMock>* media_navi_app_ptr_;
  uint32_t media_navi_app_id_ = 1855;

  am::ApplicationSharedPtr media_vc_app_;
  NiceMock<ApplicationMock>* media_vc_app_ptr_;
  uint32_t media_vc_app_id_ = 1881;

  am::ApplicationSharedPtr navi_vc_app_;
  NiceMock<ApplicationMock>* navi_vc_app_ptr_;
  uint32_t navi_vc_app_id_ = 1894;

  am::ApplicationSharedPtr media_navi_vc_app_;
  NiceMock<ApplicationMock>* media_navi_vc_app_ptr_;
  uint32_t media_navi_vc_app_id_ = 1922;

  std::vector<am::HmiStatePtr> valid_states_for_audio_app_;
  std::vector<am::HmiStatePtr> valid_states_for_not_audio_app_;
  std::vector<am::HmiStatePtr> common_invalid_states_;
  std::vector<am::HmiStatePtr> invalid_states_for_not_audio_app;
  std::vector<am::HmiStatePtr> invalid_states_for_audio_app;

  am::HmiStatePtr createHmiState(
      mobile_apis::HMILevel::eType hmi_level,
      mobile_apis::AudioStreamingState::eType aidio_ss,
      mobile_apis::SystemContext::eType system_context) {
    namespace HMILevel = mobile_apis::HMILevel;
    namespace AudioStreamingState = mobile_apis::AudioStreamingState;
    namespace SystemContext = mobile_apis::SystemContext;

    am::HmiStatePtr state = utils::MakeShared<am::HmiState>(
        simple_app_id_, state_ctrl_.state_context());
    state->set_hmi_level(hmi_level);
    state->set_audio_streaming_state(aidio_ss);
    state->set_system_context(system_context);
    return state;
  }

 protected:
  am::ApplicationSharedPtr ConfigureApp(NiceMock<ApplicationMock>** app_mock,
                                        uint32_t app_id, bool media, bool navi,
                                        bool vc) {
    *app_mock = new NiceMock<ApplicationMock>;

    Mock::AllowLeak(*app_mock);  // WorkAround for gogletest bug
    am::ApplicationSharedPtr app(*app_mock);

    ON_CALL(**app_mock, app_id()).WillByDefault(Return(app_id));
    ON_CALL(**app_mock, is_media_application()).WillByDefault(Return(media));
    ON_CALL(**app_mock, is_navi()).WillByDefault(Return(navi));
    ON_CALL(**app_mock, is_voice_communication_supported())
        .WillByDefault(Return(vc));
    ON_CALL(**app_mock, IsAudioApplication())
        .WillByDefault(Return(media || navi || vc));

    EXPECT_CALL(**app_mock, usage_report())
        .WillRepeatedly(ReturnRef(usage_stat));

    return app;
  }

  void FillStatesLists() {
    namespace HMILevel = mobile_apis::HMILevel;
    namespace AudioStreamingState = mobile_apis::AudioStreamingState;
    namespace SystemContext = mobile_apis::SystemContext;
    // Valid states for not audio app
    message_helper_mock_ = new MessageHelperMock;
    valid_states_for_not_audio_app_.push_back(
        createHmiState(HMILevel::HMI_NONE, AudioStreamingState::NOT_AUDIBLE,
                       SystemContext::SYSCTXT_MAIN));
    valid_states_for_not_audio_app_.push_back(
        createHmiState(HMILevel::HMI_NONE, AudioStreamingState::NOT_AUDIBLE,
                       SystemContext::SYSCTXT_VRSESSION));
    valid_states_for_not_audio_app_.push_back(
        createHmiState(HMILevel::HMI_NONE, AudioStreamingState::NOT_AUDIBLE,
                       SystemContext::SYSCTXT_MENU));
    valid_states_for_not_audio_app_.push_back(
        createHmiState(HMILevel::HMI_NONE, AudioStreamingState::NOT_AUDIBLE,
                       SystemContext::SYSCTXT_HMI_OBSCURED));
    valid_states_for_not_audio_app_.push_back(
        createHmiState(HMILevel::HMI_NONE, AudioStreamingState::NOT_AUDIBLE,
                       SystemContext::SYSCTXT_ALERT));
    valid_states_for_not_audio_app_.push_back(createHmiState(
        HMILevel::HMI_BACKGROUND, AudioStreamingState::NOT_AUDIBLE,
        SystemContext::SYSCTXT_MAIN));
    valid_states_for_not_audio_app_.push_back(
        createHmiState(HMILevel::HMI_FULL, AudioStreamingState::NOT_AUDIBLE,
                       SystemContext::SYSCTXT_MAIN));

    // Valid states audio app
    valid_states_for_audio_app_.push_back(
        createHmiState(HMILevel::HMI_NONE, AudioStreamingState::NOT_AUDIBLE,
                       SystemContext::SYSCTXT_MAIN));
    valid_states_for_audio_app_.push_back(
        createHmiState(HMILevel::HMI_NONE, AudioStreamingState::NOT_AUDIBLE,
                       SystemContext::SYSCTXT_VRSESSION));
    valid_states_for_audio_app_.push_back(
        createHmiState(HMILevel::HMI_NONE, AudioStreamingState::NOT_AUDIBLE,
                       SystemContext::SYSCTXT_MENU));
    valid_states_for_audio_app_.push_back(
        createHmiState(HMILevel::HMI_NONE, AudioStreamingState::NOT_AUDIBLE,
                       SystemContext::SYSCTXT_HMI_OBSCURED));
    valid_states_for_audio_app_.push_back(
        createHmiState(HMILevel::HMI_NONE, AudioStreamingState::NOT_AUDIBLE,
                       SystemContext::SYSCTXT_ALERT));
    valid_states_for_audio_app_.push_back(createHmiState(
        HMILevel::HMI_BACKGROUND, AudioStreamingState::NOT_AUDIBLE,
        SystemContext::SYSCTXT_MAIN));
    valid_states_for_audio_app_.push_back(
        createHmiState(HMILevel::HMI_LIMITED, AudioStreamingState::AUDIBLE,
                       SystemContext::SYSCTXT_MAIN));
    valid_states_for_audio_app_.push_back(
        createHmiState(HMILevel::HMI_LIMITED, AudioStreamingState::ATTENUATED,
                       SystemContext::SYSCTXT_MAIN));
    valid_states_for_audio_app_.push_back(
        createHmiState(HMILevel::HMI_FULL, AudioStreamingState::NOT_AUDIBLE,
                       SystemContext::SYSCTXT_MAIN));
    valid_states_for_audio_app_.push_back(
        createHmiState(HMILevel::HMI_FULL, AudioStreamingState::AUDIBLE,
                       SystemContext::SYSCTXT_MAIN));

    // Common Invalid States
    common_invalid_states_.push_back(
        createHmiState(HMILevel::INVALID_ENUM, AudioStreamingState::NOT_AUDIBLE,
                       SystemContext::SYSCTXT_MAIN));
    common_invalid_states_.push_back(
        createHmiState(HMILevel::HMI_NONE, AudioStreamingState::INVALID_ENUM,
                       SystemContext::SYSCTXT_MAIN));
    common_invalid_states_.push_back(
        createHmiState(HMILevel::HMI_NONE, AudioStreamingState::NOT_AUDIBLE,
                       SystemContext::INVALID_ENUM));
    common_invalid_states_.push_back(createHmiState(
        HMILevel::INVALID_ENUM, AudioStreamingState::INVALID_ENUM,
        SystemContext::SYSCTXT_MAIN));
    common_invalid_states_.push_back(
        createHmiState(HMILevel::HMI_NONE, AudioStreamingState::INVALID_ENUM,
                       SystemContext::INVALID_ENUM));
    common_invalid_states_.push_back(createHmiState(
        HMILevel::INVALID_ENUM, AudioStreamingState::INVALID_ENUM,
        SystemContext::INVALID_ENUM));
    common_invalid_states_.push_back(createHmiState(
        HMILevel::INVALID_ENUM, AudioStreamingState::INVALID_ENUM,
        SystemContext::INVALID_ENUM));
    // Invalid States for audio apps
    invalid_states_for_audio_app.push_back(
        createHmiState(HMILevel::HMI_LIMITED, AudioStreamingState::NOT_AUDIBLE,
                       SystemContext::SYSCTXT_MAIN));
    invalid_states_for_audio_app.push_back(
        createHmiState(HMILevel::HMI_BACKGROUND, AudioStreamingState::AUDIBLE,
                       SystemContext::SYSCTXT_MAIN));
    invalid_states_for_audio_app.push_back(createHmiState(
        HMILevel::HMI_BACKGROUND, AudioStreamingState::ATTENUATED,
        SystemContext::SYSCTXT_MAIN));
    invalid_states_for_audio_app.push_back(
        createHmiState(HMILevel::HMI_NONE, AudioStreamingState::AUDIBLE,
                       SystemContext::SYSCTXT_MAIN));
    invalid_states_for_audio_app.push_back(
        createHmiState(HMILevel::HMI_NONE, AudioStreamingState::ATTENUATED,
                       SystemContext::SYSCTXT_MAIN));
    invalid_states_for_audio_app.push_back(
        createHmiState(HMILevel::HMI_NONE, AudioStreamingState::ATTENUATED,
                       SystemContext::SYSCTXT_MAIN));
    // Invalid States for not audio apps
    invalid_states_for_not_audio_app.push_back(
        createHmiState(HMILevel::HMI_LIMITED, AudioStreamingState::ATTENUATED,
                       SystemContext::SYSCTXT_MAIN));
    invalid_states_for_not_audio_app.push_back(
        createHmiState(HMILevel::HMI_LIMITED, AudioStreamingState::AUDIBLE,
                       SystemContext::SYSCTXT_MAIN));
    invalid_states_for_not_audio_app.push_back(
        createHmiState(HMILevel::HMI_FULL, AudioStreamingState::ATTENUATED,
                       SystemContext::SYSCTXT_MAIN));
    invalid_states_for_not_audio_app.push_back(
        createHmiState(HMILevel::HMI_FULL, AudioStreamingState::AUDIBLE,
                       SystemContext::SYSCTXT_MAIN));
  }

  void ConfigureApps() {
    simple_app_ = ConfigureApp(&simple_app_ptr_, simple_app_id_, NOT_MEDIA,
                               NOT_NAVI, NOT_VC);
    media_app_ =
        ConfigureApp(&media_app_ptr_, media_app_id_, MEDIA, NOT_NAVI, NOT_VC);
    navi_app_ =
        ConfigureApp(&navi_app_ptr_, navi_app_id_, NOT_MEDIA, NAVI, NOT_VC);
    vc_app_ = ConfigureApp(&vc_app_ptr_, vc_app_id_, NOT_MEDIA, NOT_NAVI, VC);
    media_navi_app_ = ConfigureApp(&media_navi_app_ptr_, media_navi_app_id_,
                                   MEDIA, NAVI, NOT_VC);
    media_vc_app_ =
        ConfigureApp(&media_vc_app_ptr_, media_vc_app_id_, MEDIA, NOT_NAVI, VC);
    navi_vc_app_ =
        ConfigureApp(&navi_vc_app_ptr_, navi_vc_app_id_, NOT_MEDIA, NAVI, VC);
    media_navi_vc_app_ = ConfigureApp(&media_navi_vc_app_ptr_,
                                      media_navi_vc_app_id_, MEDIA, NAVI, VC);
  }
  void CheckAppConfiguration() {
    ASSERT_EQ(simple_app_.get(), simple_app_ptr_);
    ASSERT_EQ(media_app_.get(), media_app_ptr_);
    ASSERT_EQ(navi_app_.get(), navi_app_ptr_);
    ASSERT_EQ(vc_app_.get(), vc_app_ptr_);
    ASSERT_EQ(media_navi_app_.get(), media_navi_app_ptr_);
    ASSERT_EQ(media_vc_app_.get(), media_vc_app_ptr_);
    ASSERT_EQ(navi_vc_app_.get(), navi_vc_app_ptr_);
    ASSERT_EQ(media_navi_vc_app_.get(), media_navi_vc_app_ptr_);

    ASSERT_EQ(simple_app_->app_id(), simple_app_id_);
    ASSERT_EQ(media_app_->app_id(), media_app_id_);
    ASSERT_EQ(navi_app_->app_id(), navi_app_id_);
    ASSERT_EQ(vc_app_->app_id(), vc_app_id_);
    ASSERT_EQ(media_navi_app_->app_id(), media_navi_app_id_);
    ASSERT_EQ(media_vc_app_->app_id(), media_vc_app_id_);
    ASSERT_EQ(navi_vc_app_->app_id(), navi_vc_app_id_);
    ASSERT_EQ(media_navi_vc_app_->app_id(), media_navi_vc_app_id_);

    ASSERT_FALSE(simple_app_->IsAudioApplication());
    ASSERT_TRUE(media_app_->IsAudioApplication());
    ASSERT_TRUE(navi_app_->IsAudioApplication());
    ASSERT_TRUE(vc_app_->IsAudioApplication());
    ASSERT_TRUE(media_navi_app_->IsAudioApplication());
    ASSERT_TRUE(media_vc_app_->IsAudioApplication());
    ASSERT_TRUE(navi_vc_app_->IsAudioApplication());
    ASSERT_TRUE(media_navi_vc_app_->IsAudioApplication());

    ASSERT_FALSE(simple_app_->is_media_application());
    ASSERT_TRUE(media_app_->is_media_application());
    ASSERT_FALSE(navi_app_->is_media_application());
    ASSERT_FALSE(vc_app_->is_media_application());
    ASSERT_TRUE(media_navi_app_->is_media_application());
    ASSERT_TRUE(media_vc_app_->is_media_application());
    ASSERT_FALSE(navi_vc_app_->is_media_application());
    ASSERT_TRUE(media_navi_vc_app_->is_media_application());

    ASSERT_FALSE(simple_app_->is_navi());
    ASSERT_TRUE(navi_app_->is_navi());
    ASSERT_FALSE(media_app_->is_navi());
    ASSERT_FALSE(vc_app_->is_navi());
    ASSERT_TRUE(media_navi_app_->is_navi());
    ASSERT_FALSE(media_vc_app_->is_navi());
    ASSERT_TRUE(navi_vc_app_->is_navi());
    ASSERT_TRUE(media_navi_vc_app_->is_navi());

    ASSERT_FALSE(simple_app_->is_voice_communication_supported());
    ASSERT_FALSE(navi_app_->is_voice_communication_supported());
    ASSERT_FALSE(media_app_->is_voice_communication_supported());
    ASSERT_TRUE(vc_app_->is_voice_communication_supported());
    ASSERT_FALSE(media_navi_app_->is_voice_communication_supported());
    ASSERT_TRUE(media_vc_app_->is_voice_communication_supported());
    ASSERT_TRUE(navi_vc_app_->is_voice_communication_supported());
    ASSERT_TRUE(media_navi_vc_app_->is_voice_communication_supported());
  }

  void SetUp() {
    ON_CALL(app_manager_mock_, applications())
        .WillByDefault(Return(applications_));
    ConfigureApps();
    CheckAppConfiguration();
    FillStatesLists();
  }

  void TearDown() { delete message_helper_mock_; }

  void ExpectSuccesfullSetHmiState(am::ApplicationSharedPtr app,
                                   NiceMock<ApplicationMock>* app_mock,
                                   am::HmiStatePtr old_state,
                                   am::HmiStatePtr new_state) {
    EXPECT_CALL(*app_mock, CurrentHmiState())
        .WillOnce(Return(old_state))
        .WillOnce(Return(new_state));
    EXPECT_CALL(*app_mock,
                SetRegularState(Truly(HmiStatesComparator(new_state))));
    if (!HmiStatesComparator(old_state)(new_state)) {
      EXPECT_CALL(app_manager_mock_, SendHMIStatusNotification(app));
      EXPECT_CALL(app_manager_mock_,
                  OnHMILevelChanged(app->app_id(), old_state->hmi_level(),
                                    new_state->hmi_level()));
    }
  }

  void ExpectAppChangeHmiStateDueToConflictResolving(
      am::ApplicationSharedPtr app, NiceMock<ApplicationMock>* app_mock,
      am::HmiStatePtr old_state, am::HmiStatePtr new_state) {
    EXPECT_CALL(*app_mock, RegularHmiState())
        .WillOnce(Return(old_state))
        .WillOnce(Return(old_state));
    ExpectSuccesfullSetHmiState(app, app_mock, old_state, new_state);
  }

  void ExpectAppWontChangeHmiStateDueToConflictResolving(
      am::ApplicationSharedPtr app, NiceMock<ApplicationMock>* app_mock,
      am::HmiStatePtr state) {
    EXPECT_CALL(*app_mock, RegularHmiState()).WillOnce(Return(state));
    EXPECT_CALL(app_manager_mock_, SendHMIStatusNotification(app)).Times(0);
    EXPECT_CALL(app_manager_mock_, OnHMILevelChanged(app->app_id(), _, _))
        .Times(0);
  }

  void InsertApplication(am::ApplicationSharedPtr app) {
    application_set_.insert(app);
    ON_CALL(app_manager_mock_, application(app->app_id()))
        .WillByDefault(Return(app));
  }

  am::HmiStatePtr FullAudibleState() {
    return createHmiState(mobile_apis::HMILevel::HMI_FULL,
                          mobile_apis::AudioStreamingState::AUDIBLE,
                          mobile_apis::SystemContext::SYSCTXT_MAIN);
  }

  am::HmiStatePtr FullNotAudibleState() {
    return createHmiState(mobile_apis::HMILevel::HMI_FULL,
                          mobile_apis::AudioStreamingState::NOT_AUDIBLE,
                          mobile_apis::SystemContext::SYSCTXT_MAIN);
  }

  am::HmiStatePtr LimitedState() {
    return createHmiState(mobile_apis::HMILevel::HMI_LIMITED,
                          mobile_apis::AudioStreamingState::AUDIBLE,
                          mobile_apis::SystemContext::SYSCTXT_MAIN);
  }

  am::HmiStatePtr BackgroundState() {
    return createHmiState(mobile_apis::HMILevel::HMI_BACKGROUND,
                          mobile_apis::AudioStreamingState::NOT_AUDIBLE,
                          mobile_apis::SystemContext::SYSCTXT_MAIN);
  }
};


}  // namespace state_controller_test
