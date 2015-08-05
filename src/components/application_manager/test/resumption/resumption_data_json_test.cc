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

#include <string>
#include <sstream>
#include <algorithm>
#include "gtest/gtest.h"
#include "application_manager/resumption/resumption_data_json.h"
#include "application_manager/usage_statistics.h"
#include "include/application_mock.h"
#include "include/resumption_data_mock.h"
#include "interfaces/MOBILE_API.h"
#include "resumption/last_state.h"

#include "application_manager/application_manager_impl.h"
#include "application_manager/application.h"
#include "utils/data_accessor.h"
#include "application_manager/message_helper.h"
#include "formatters/CFormatterJsonBase.hpp"
#include "config_profile/profile.h"
#include "utils/file_system.h"

std::string application_manager::MessageHelper::GetDeviceMacAddressForHandle(
    const uint32_t device_handle) {
  std::string device_mac_address = "12345";
  return device_mac_address;
}

namespace test {
namespace components {
namespace resumption_test {

using ::testing::_;
using ::testing::Return;
using ::testing::DoAll;
using ::testing::NiceMock;
using ::testing::ReturnRef;
using ::testing::ReturnPointee;

namespace am = application_manager;
using namespace Json;
using namespace file_system;

using namespace resumption;
using namespace mobile_apis;
namespace Formatters = NsSmartDeviceLink::NsJSONHandler::Formatters;

class ResumptionDataJsonTest : public ::testing::Test {
 protected:
  virtual void SetUp() {
    app_mock = new NiceMock<ApplicationMock>();

    policy_app_id_ = "test_policy_app_id";
    app_id_ = 10;
    is_audio_ = true;
    grammar_id_ = 20;
    hash_ = "saved_hash";
    hmi_level_ = HMILevel::eType::HMI_FULL;
    device_ = 10;
    hmi_app_id_ = 8;
    ign_off_count_ = 0;
    suspend_count_ = 0;
  }

  // Check structure in saved dictionary
  void CheckSavedJson();
  void CheckSavedJson(Value& saved_data);

  // Set data for resumption
  void PrepareData();
  utils::SharedPtr<NiceMock<ApplicationMock>> app_mock;

  HMILevel::eType hmi_level_;
  uint32_t app_id_;
  uint32_t hmi_app_id_;
  std::string policy_app_id_;
  uint32_t ign_off_count_;
  uint32_t suspend_count_;
  uint32_t grammar_id_;
  std::string hash_;
  bool is_audio_;
  connection_handler::DeviceHandle device_;

  smart_objects::SmartObject* help_prompt_;
  smart_objects::SmartObject* timeout_prompt_;
  smart_objects::SmartObject* vr_help_;
  smart_objects::SmartObject* vr_help_title_;
  smart_objects::SmartObject* vr_synonyms_;
  smart_objects::SmartObject* keyboard_props_;
  smart_objects::SmartObject* menu_title_;
  smart_objects::SmartObject* menu_icon_;

 private:
  void SetCommands();
  void SetSubmenues();
  void SetChoiceSet();
  void SetAppFiles();
  void SetGlobalProporties();
  void SetKeyboardProperties();
  void SetMenuTitleAndIcon();
  void SetHelpAndTimeoutPrompt();
  void SetVRHelpTitleSynonyms();
  void SetSubscriptions();

  void CheckCommands(Value& res_list);
  void CheckGlobalProporties(Value& res_list);
  void CheckSubmenues(Value& res_list);
  void CheckChoiceSet(Value& res_list);
  void CheckAppFiles(Value& res_list);
  void CheckKeyboardProperties(Value& res_list);
  void CheckMenuTitle(Value& res_list);
  void CheckMenuIcon(Value& res_list);
  void CheckHelpPrompt(Value& res_list);
  void CheckTimeoutPrompt(Value& res_list);
  void CheckVRHelp(Value& res_list);
  void CheckVRTitle(Value& res_list);
  void CheckSubscriptions(Value& res_list);

  const u_int32_t count_of_commands = 5;
  const size_t count_of_choice = 2;
  const size_t count_of_choice_sets = 4;
  const u_int32_t count_of_submenues = 3;
  const u_int32_t count_of_files = 8;

  am::CommandsMap test_commands_map;
  am::SubMenuMap test_submenu_map;
  am::ChoiceSetMap test_choiceset_map;
  am::AppFilesMap app_files_map_;

  am::ButtonSubscriptions btn_subscr;
  am::VehicleInfoSubscriptions ivi;

  sync_primitives::Lock sublock_;
  sync_primitives::Lock comlock_;
  sync_primitives::Lock setlock_;
  sync_primitives::Lock btnlock_;
  sync_primitives::Lock ivilock_;
};

void ResumptionDataJsonTest::CheckSavedJson() {
  Value& dictionary = LastState::instance()->dictionary;
  ASSERT_TRUE(dictionary[am::strings::resumption].isObject());
  ASSERT_TRUE(dictionary[am::strings::resumption][am::strings::resume_app_list]
                  .isArray());

  Value& resume_app_list =
      dictionary[am::strings::resumption][am::strings::resume_app_list];
  for (uint i = 0; i < resume_app_list.size(); i++) {
    CheckSavedJson(resume_app_list[i]);
  }
}

void ResumptionDataJsonTest::CheckSavedJson(Value& resume_app_list) {
  EXPECT_EQ(policy_app_id_, resume_app_list[am::strings::app_id].asString());
  EXPECT_EQ(grammar_id_, resume_app_list[am::strings::grammar_id].asUInt());
  EXPECT_EQ(app_id_, resume_app_list[am::strings::connection_key].asUInt());
  EXPECT_EQ(hmi_app_id_, resume_app_list[am::strings::hmi_app_id].asUInt());
  EXPECT_EQ(ign_off_count_,
            resume_app_list[am::strings::ign_off_count].asUInt());
  EXPECT_EQ(hmi_level_, static_cast<HMILevel::eType>(
                            resume_app_list[am::strings::hmi_level].asInt()));
  EXPECT_EQ(is_audio_,
            resume_app_list[am::strings::is_media_application].asBool());
  EXPECT_EQ("12345", resume_app_list[am::strings::device_id].asString());

  CheckCommands(resume_app_list[am::strings::application_commands]);
  CheckSubmenues(resume_app_list[am::strings::application_submenus]);
  CheckChoiceSet(resume_app_list[am::strings::application_choice_sets]);
  CheckAppFiles(resume_app_list[am::strings::application_files]);

  CheckGlobalProporties(
      resume_app_list[am::strings::application_global_properties]);
  CheckSubscriptions(resume_app_list[am::strings::application_subscribtions]);
}

void ResumptionDataJsonTest::CheckCommands(Value& res_list) {
  for (u_int32_t i = 0; i < count_of_commands; ++i) {
    EXPECT_EQ(i, res_list[i][am::strings::cmd_id].asUInt());
  }
}

void ResumptionDataJsonTest::CheckSubmenues(Value& res_list) {
  for (u_int32_t i = 0; i < count_of_submenues; ++i) {
    EXPECT_EQ(i + 10, res_list[i][am::strings::menu_id].asUInt());
  }
}

void ResumptionDataJsonTest::CheckSubscriptions(Value& res_list) {
  EXPECT_EQ(ButtonName::eType::OK,
            res_list[am::strings::application_buttons][0].asUInt());
  EXPECT_EQ(ButtonName::eType::CUSTOM_BUTTON,
            res_list[am::strings::application_buttons][1].asUInt());
  EXPECT_EQ(0, res_list[am::strings::application_vehicle_info][0].asUInt());
  EXPECT_EQ(5, res_list[am::strings::application_vehicle_info][1].asUInt());
}

void ResumptionDataJsonTest::CheckChoiceSet(Value& res_list) {
  for (u_int32_t i = 0; i < res_list.size(); ++i) {
    for (u_int32_t j = 0; j < res_list[i][am::strings::choice_set].size();
         ++j) {
      Value command = res_list[i][am::strings::choice_set][j];
      EXPECT_EQ(i + j, command[am::strings::choice_id].asUInt());
      char numb[12];
      std::snprintf(numb, 12,"%d",i+j);
      EXPECT_EQ("VrCommand " + std::string(numb) ,
                command[am::strings::vr_commands][0].asString());
    }
    EXPECT_EQ(i, res_list[i][am::strings::interaction_choice_set_id].asUInt());
  }
}

void ResumptionDataJsonTest::CheckAppFiles(Value& res_list) {
  am::AppFile check_file;

  for (uint i = 0; i < count_of_files; ++i) {
    char numb[12];
    std::snprintf(numb, 12,"%d",i);
    check_file = app_files_map_["test_file " +std::string(numb)];
    EXPECT_EQ(check_file.file_name,
              res_list[i][am::strings::sync_file_name].asString());
    EXPECT_EQ(check_file.file_type,
              static_cast<FileType::eType>(
                  res_list[i][am::strings::file_type].asInt()));
    EXPECT_EQ(check_file.is_download_complete,
              res_list[i][am::strings::is_download_complete].asBool());
    EXPECT_EQ(check_file.is_persistent,
              res_list[i][am::strings::persistent_file].asBool());
  }
}

void ResumptionDataJsonTest::CheckGlobalProporties(Value& res_list) {
  CheckHelpPrompt(res_list[am::strings::help_prompt]);
  CheckTimeoutPrompt(res_list[am::strings::timeout_prompt]);
  CheckVRHelp(res_list[am::strings::vr_help]);
  CheckVRTitle(res_list[am::strings::vr_help_title]);
  CheckKeyboardProperties(res_list[am::strings::keyboard_properties]);
  CheckMenuTitle(res_list[am::strings::menu_title]);
  CheckMenuIcon(res_list[am::strings::menu_icon]);
}

void ResumptionDataJsonTest::CheckKeyboardProperties(Value& res_list) {
  Language::eType testlanguage = static_cast<Language::eType>(
      (*keyboard_props_)[am::strings::language].asInt());
  KeyboardLayout::eType testlayout = static_cast<KeyboardLayout::eType>(
      (*keyboard_props_)[am::hmi_request::keyboard_layout].asInt());
  KeypressMode::eType testmode = static_cast<KeypressMode::eType>(
      (*keyboard_props_)[am::strings::key_press_mode].asInt());

  EXPECT_EQ(testlanguage, static_cast<Language::eType>(
                              res_list[am::strings::language].asInt()));
  EXPECT_EQ(testlayout,
            static_cast<KeyboardLayout::eType>(
                res_list[am::hmi_request::keyboard_layout].asInt()));
  EXPECT_EQ(testmode, static_cast<KeypressMode::eType>(
                          res_list[am::strings::key_press_mode].asInt()));
}

void ResumptionDataJsonTest::CheckMenuTitle(Value& res_list) {
  std::string value = (*menu_title_)[am::strings::menu_title].asString();
  EXPECT_EQ(value, res_list[am::strings::menu_title].asString());
}

void ResumptionDataJsonTest::CheckMenuIcon(Value& res_list) {
  std::string value = (*menu_icon_)[am::strings::value].asString();
  ImageType::eType type = static_cast<ImageType::eType>(
      (*menu_icon_)[am::strings::image_type].asInt());

  EXPECT_EQ(value, res_list[am::strings::value].asString());

  EXPECT_EQ(type, static_cast<ImageType::eType>(
                      res_list[am::strings::image_type].asInt()));
}

void ResumptionDataJsonTest::CheckHelpPrompt(Value& res_list) {
  std::string help_prompt_text =
      (*help_prompt_)[0][am::strings::help_prompt].asString();
  std::string dict_help_promt =
      res_list[0][am::strings::help_prompt].asString();
  EXPECT_EQ(help_prompt_text, dict_help_promt);
}

void ResumptionDataJsonTest::CheckTimeoutPrompt(Value& res_list) {
  std::string text = (*timeout_prompt_)[0][am::strings::text].asString();
  SpeechCapabilities::eType speech = static_cast<SpeechCapabilities::eType>(
      (*timeout_prompt_)[0][am::strings::type].asInt());
  EXPECT_EQ(text, res_list[0][am::strings::text].asString());
  EXPECT_EQ(speech, static_cast<SpeechCapabilities::eType>(
                        res_list[0][am::strings::type].asInt()));
}

void ResumptionDataJsonTest::CheckVRHelp(Value& res_list) {
  std::string text = (*vr_help_)[0][am::strings::text].asString();
  EXPECT_EQ(text, res_list[0][am::strings::text].asString());
  int position = (*vr_help_)[0][am::strings::position].asInt();
  EXPECT_EQ(position, res_list[0][am::strings::position].asInt());
}

void ResumptionDataJsonTest::CheckVRTitle(Value& res_list) {
  std::string vr_help_title =
      (*vr_help_title_)[am::strings::vr_help_title].asString();
  EXPECT_EQ(vr_help_title, res_list[am::strings::vr_help_title].asString());
}

void ResumptionDataJsonTest::PrepareData() {
  SetGlobalProporties();
  SetCommands();
  SetSubmenues();
  SetChoiceSet();
  SetAppFiles();

  DataAccessor<am::SubMenuMap> sub_menu_m(test_submenu_map, sublock_);
  DataAccessor<am::CommandsMap> commands_m(test_commands_map, comlock_);
  DataAccessor<am::ChoiceSetMap> choice_set_m(test_choiceset_map, setlock_);

  SetSubscriptions();
  DataAccessor<am::ButtonSubscriptions> btn_sub(btn_subscr, btnlock_);
  DataAccessor<am::VehicleInfoSubscriptions> ivi_access(ivi, ivilock_);

  ON_CALL(*app_mock, policy_app_id()).WillByDefault(Return(policy_app_id_));
  ON_CALL(*app_mock, curHash()).WillByDefault(ReturnRef(hash_));
  ON_CALL(*app_mock, get_grammar_id()).WillByDefault(Return(grammar_id_));
  ON_CALL(*app_mock, device()).WillByDefault(Return(device_));
  ON_CALL(*app_mock, hmi_level()).WillByDefault(Return(hmi_level_));
  ON_CALL(*app_mock, app_id()).WillByDefault(Return(app_id_));
  ON_CALL(*app_mock, hmi_app_id()).WillByDefault(Return(hmi_app_id_));
  ON_CALL(*app_mock, IsAudioApplication()).WillByDefault(Return(is_audio_));
  ON_CALL(*app_mock, commands_map()).WillByDefault(Return(commands_m));
  ON_CALL(*app_mock, sub_menu_map()).WillByDefault(Return(sub_menu_m));
  ON_CALL(*app_mock, choice_set_map()).WillByDefault(Return(choice_set_m));

  ON_CALL(*app_mock, help_prompt()).WillByDefault(ReturnPointee(&help_prompt_));
  ON_CALL(*app_mock, timeout_prompt())
      .WillByDefault(ReturnPointee(&timeout_prompt_));
  ON_CALL(*app_mock, vr_help()).WillByDefault(ReturnPointee(&vr_help_));
  ON_CALL(*app_mock, vr_help_title())
      .WillByDefault(ReturnPointee(&vr_help_title_));


  ON_CALL(*app_mock, keyboard_props())
      .WillByDefault(ReturnPointee(&keyboard_props_));
  ON_CALL(*app_mock, menu_title()).WillByDefault(ReturnPointee(&menu_title_));
  ON_CALL(*app_mock, menu_icon()).WillByDefault(ReturnPointee(&menu_icon_));

  ON_CALL(*app_mock, SubscribedButtons()).WillByDefault(Return(btn_sub));
  ON_CALL(*app_mock, SubscribedIVI()).WillByDefault(Return(ivi_access));

  ON_CALL(*app_mock, getAppFiles()).WillByDefault(ReturnRef(app_files_map_));
}

void ResumptionDataJsonTest::SetGlobalProporties() {
  SetKeyboardProperties();
  SetMenuTitleAndIcon();
  SetHelpAndTimeoutPrompt();
  SetVRHelpTitleSynonyms();
}

void ResumptionDataJsonTest::SetMenuTitleAndIcon() {
  smart_objects::SmartObject sm_icon;
  sm_icon[am::strings::value] = "test icon";
  sm_icon[am::strings::image_type] = ImageType::STATIC;

  smart_objects::SmartObject sm_title;
  sm_title[am::strings::menu_title] = "test title";
  menu_title_ = new smart_objects::SmartObject(sm_title);
  menu_icon_ = new smart_objects::SmartObject(sm_icon);
}

void ResumptionDataJsonTest::SetHelpAndTimeoutPrompt() {
  smart_objects::SmartObject help_prompt;
  help_prompt[0][am::strings::help_prompt] = "help prompt name";
  help_prompt_ = new smart_objects::SmartObject(help_prompt);

  smart_objects::SmartObject timeout_prompt;
  timeout_prompt[0][am::strings::text] = "timeout test";
  timeout_prompt[0][am::strings::type] = SpeechCapabilities::SC_TEXT;
  timeout_prompt_ = new smart_objects::SmartObject(timeout_prompt);
}

void ResumptionDataJsonTest::SetVRHelpTitleSynonyms() {
  smart_objects::SmartObject vr_help_title;
  vr_help_title[am::strings::vr_help_title] = "vr help title";

  smart_objects::SmartObject vr_help;
  vr_help[0][am::strings::text] = "vr help";
  vr_help[0][am::strings::position] = 1;
  vr_help_ = new smart_objects::SmartObject(vr_help);
  vr_help_title_ = new smart_objects::SmartObject(vr_help_title);
}

void ResumptionDataJsonTest::SetCommands() {
  smart_objects::SmartObject sm_comm;
  for (u_int32_t i = 0; i < count_of_commands; ++i) {
    sm_comm[am::strings::cmd_id] = i;
    test_commands_map[i] = new smart_objects::SmartObject(sm_comm);
  }
}

void ResumptionDataJsonTest::SetSubmenues() {
  smart_objects::SmartObject sm_comm;
  for (u_int32_t i = 10; i < count_of_submenues + 10; ++i) {
    sm_comm[am::strings::menu_id] = i;
    test_submenu_map[i] = new smart_objects::SmartObject(sm_comm);
  }
}

void ResumptionDataJsonTest::SetChoiceSet() {
  smart_objects::SmartObject choice_vector;
  smart_objects::SmartObject choice;
  smart_objects::SmartObject vr_commandsvector;
  smart_objects::SmartObject app_choice_set;
  smart_objects::SmartObject application_choice_sets;
  for (u_int32_t i = 0; i < count_of_choice_sets; ++i) {
    for (u_int32_t j = 0; j < count_of_choice; ++j) {
      char numb[12];
      std::snprintf(numb, 12,"%d",i+j);

      choice[am::strings::choice_id] = i + j;
      vr_commandsvector[0] = "VrCommand " + std::string(numb);
      choice[am::strings::vr_commands] = vr_commandsvector;
      choice_vector[j] = choice;
    }
    app_choice_set[am::strings::interaction_choice_set_id] = i;
    app_choice_set[am::strings::choice_set] = choice_vector;
    application_choice_sets[i] = app_choice_set;

    test_choiceset_map[i] =
        new smart_objects::SmartObject(application_choice_sets[i]);
  }
}

void ResumptionDataJsonTest::SetAppFiles() {
  am::AppFile test_file;
  int file_types;
  std::string file_names;
  for (uint i = 0; i < count_of_files; ++i) {
    char numb[12];
    std::snprintf(numb, 12,"%d",i);
    file_types = i;
    file_names = "test_file " + std::string(numb);
    test_file.is_persistent = true;
    test_file.is_download_complete = true;
    test_file.file_type = static_cast<FileType::eType>(file_types);
    test_file.file_name = file_names;

    app_files_map_[test_file.file_name] = test_file;
  }
}

void ResumptionDataJsonTest::SetKeyboardProperties() {
  smart_objects::SmartObject keyboard;
  keyboard[am::strings::language] = Language::EN_US;
  keyboard[am::hmi_request::keyboard_layout] = KeyboardLayout::QWERTY;
  keyboard[am::strings::key_press_mode] = KeypressMode::SINGLE_KEYPRESS;
  keyboard_props_ = new smart_objects::SmartObject(keyboard);
}

void ResumptionDataJsonTest::SetSubscriptions() {
  btn_subscr.insert(ButtonName::eType::CUSTOM_BUTTON);
  btn_subscr.insert(ButtonName::eType::OK);
  ivi.insert(0);
  ivi.insert(5);
}

TEST_F(ResumptionDataJsonTest, SaveApplication) {
  ResumptionDataJson res_json;
  PrepareData();
  res_json.SaveApplication(app_mock);
  CheckSavedJson();
}

TEST_F(ResumptionDataJsonTest, SavedApplicationTwice) {
  ResumptionDataJson res_json;
  PrepareData();
  res_json.SaveApplication(app_mock);
  CheckSavedJson();
  res_json.SaveApplication(app_mock);
  CheckSavedJson();
}

TEST_F(ResumptionDataJsonTest, SavedApplicationTwice_UpdateApp) {
  ResumptionDataJson res_json;
  PrepareData();
  res_json.SaveApplication(app_mock);
  CheckSavedJson();
  (*vr_help_)[0][am::strings::position] = 2;

  res_json.SaveApplication(app_mock);
  CheckSavedJson();
}

TEST_F(ResumptionDataJsonTest, RemoveApplicationFromSaved) {
  ResumptionDataJson res_json;
  PrepareData();
  res_json.SaveApplication(app_mock);
  EXPECT_TRUE(res_json.RemoveApplicationFromSaved(policy_app_id_, "12345"));

  // Check that application was deleted
  smart_objects::SmartObject remove_app;
  EXPECT_FALSE(
      res_json.GetSavedApplication(policy_app_id_, "12345", remove_app));
  EXPECT_TRUE(remove_app.empty());
}

TEST_F(ResumptionDataJsonTest, RemoveApplicationFromSaved_AppNotSaved) {
  ResumptionDataJson res_json;
  EXPECT_FALSE(res_json.RemoveApplicationFromSaved(policy_app_id_, "54321"));
}

TEST_F(ResumptionDataJsonTest, IsApplicationSaved_ApplicationSaved) {
  ResumptionDataJson res_json;
  PrepareData();
  res_json.SaveApplication(app_mock);
  CheckSavedJson();
  ssize_t result = res_json.IsApplicationSaved(policy_app_id_, "12345");
  EXPECT_EQ(0, result);
}

TEST_F(ResumptionDataJsonTest, IsApplicationSaved_ApplicationRemoved) {
  ResumptionDataJson res_json;
  PrepareData();
  res_json.SaveApplication(app_mock);
  CheckSavedJson();
  EXPECT_TRUE(res_json.RemoveApplicationFromSaved(policy_app_id_, "12345"));
  ssize_t result = res_json.IsApplicationSaved(policy_app_id_, "12345");
  EXPECT_EQ(-1, result);
}

TEST_F(ResumptionDataJsonTest, GetSavedApplication) {
  ResumptionDataJson res_json;
  PrepareData();
  res_json.SaveApplication(app_mock);
  smart_objects::SmartObject saved_app;
  EXPECT_TRUE(res_json.GetSavedApplication(policy_app_id_, "12345", saved_app));
  Value json_saved_app;
  Formatters::CFormatterJsonBase::objToJsonValue(saved_app, json_saved_app);
  CheckSavedJson(json_saved_app);
}

TEST_F(ResumptionDataJsonTest, GetSavedApplication_AppNotSaved) {
  ResumptionDataJson res_json;
  smart_objects::SmartObject saved_app;
  EXPECT_FALSE(
      res_json.GetSavedApplication(policy_app_id_, "54321", saved_app));
  EXPECT_TRUE(saved_app.empty());
}

TEST_F(ResumptionDataJsonTest, GetDataForLoadResumeData) {
  ResumptionDataJson res_json;
  PrepareData();
  res_json.SaveApplication(app_mock);
  CheckSavedJson();
  smart_objects::SmartObject saved_app;
  Value json_saved_app;
  res_json.GetDataForLoadResumeData(saved_app);

  Formatters::CFormatterJsonBase::objToJsonValue(saved_app[0], json_saved_app);

  EXPECT_EQ(policy_app_id_, json_saved_app[am::strings::app_id].asString());
  EXPECT_EQ("12345", json_saved_app[am::strings::device_id].asString());
  EXPECT_EQ(hmi_level_, static_cast<HMILevel::eType>(
                            json_saved_app[am::strings::hmi_level].asInt()));
  EXPECT_EQ(ign_off_count_,
            json_saved_app[am::strings::ign_off_count].asUInt());
}

TEST_F(ResumptionDataJsonTest, GetDataForLoadResumeData_AppRemove) {
  ResumptionDataJson res_json;
  smart_objects::SmartObject saved_app;

  PrepareData();
  res_json.SaveApplication(app_mock);
  CheckSavedJson();
  EXPECT_TRUE(res_json.RemoveApplicationFromSaved(policy_app_id_, "12345"));
  res_json.GetDataForLoadResumeData(saved_app);
  EXPECT_TRUE(saved_app.empty());
}

TEST_F(ResumptionDataJsonTest, UpdateHmiLevel) {
  ResumptionDataJson res_json;
  PrepareData();
  res_json.SaveApplication(app_mock);
  CheckSavedJson();
  HMILevel::eType new_hmi_level = HMILevel::HMI_LIMITED;
  res_json.UpdateHmiLevel(policy_app_id_, "12345", new_hmi_level);
  hmi_level_ = new_hmi_level;

  CheckSavedJson();
}

TEST_F(ResumptionDataJsonTest, IsHMIApplicationIdExist_AppIsSaved) {
  ResumptionDataJson res_json;
  PrepareData();
  res_json.SaveApplication(app_mock);
  CheckSavedJson();
  EXPECT_TRUE(res_json.IsHMIApplicationIdExist(hmi_app_id_));
}

TEST_F(ResumptionDataJsonTest, IsHMIApplicationIdExist_AppNotSaved) {
  ResumptionDataJson res_json;
  PrepareData();
  res_json.SaveApplication(app_mock);

  CheckSavedJson();
  uint32_t new_hmi_app_id_ = hmi_app_id_ + 10;
  EXPECT_FALSE(res_json.IsHMIApplicationIdExist(new_hmi_app_id_));
}

TEST_F(ResumptionDataJsonTest, GetHMIApplicationID) {
  ResumptionDataJson res_json;
  PrepareData();
  res_json.SaveApplication(app_mock);
  CheckSavedJson();
  EXPECT_EQ(hmi_app_id_, res_json.GetHMIApplicationID(policy_app_id_, "12345"));
}

TEST_F(ResumptionDataJsonTest, GetHMIApplicationID_AppNotSaved) {
  ResumptionDataJson res_json;
  PrepareData();
  res_json.SaveApplication(app_mock);
  EXPECT_EQ(0, res_json.GetHMIApplicationID(policy_app_id_, "other_dev_id"));
}

TEST_F(ResumptionDataJsonTest, OnSuspend) {
  ResumptionDataJson res_json;
  PrepareData();
  res_json.SaveApplication(app_mock);
  CheckSavedJson();

  ::profile::Profile::instance()->config_file_name("smartDeviceLink_test.ini");
  res_json.OnSuspend();
  ign_off_count_++;
  suspend_count_++;
  CheckSavedJson();

  EXPECT_TRUE(FileExists("./test_app_info.dat"));
  EXPECT_TRUE(DirectoryExists("./test_storage"));
  EXPECT_TRUE(RemoveDirectory("./test_storage", true));
  EXPECT_TRUE(DeleteFile("./test_app_info.dat"));
}

TEST_F(ResumptionDataJsonTest, OnSuspendFourTimes) {
  ResumptionDataJson res_json;
  PrepareData();
  res_json.SaveApplication(app_mock);
  CheckSavedJson();

  ::profile::Profile::instance()->config_file_name("smartDeviceLink_test.ini");
  res_json.OnSuspend();
  ign_off_count_++;
  suspend_count_++;
  CheckSavedJson();

  res_json.OnSuspend();
  res_json.OnSuspend();
  res_json.OnSuspend();

  ssize_t result = res_json.IsApplicationSaved(policy_app_id_, "12345");
  EXPECT_EQ(-1, result);

  EXPECT_TRUE(FileExists("./test_app_info.dat"));
  EXPECT_TRUE(DirectoryExists("./test_storage"));
  EXPECT_TRUE(RemoveDirectory("./test_storage", true));
  EXPECT_TRUE(DeleteFile("./test_app_info.dat"));
  ::profile::Profile::instance()->config_file_name("smartDeviceLink.ini");
}

TEST_F(ResumptionDataJsonTest, OnSuspendOnAwake) {
  ResumptionDataJson res_json;
  PrepareData();
  res_json.SaveApplication(app_mock);
  CheckSavedJson();

  ::profile::Profile::instance()->config_file_name("smartDeviceLink_test.ini");
  res_json.OnSuspend();
  ign_off_count_++;
  suspend_count_++;
  CheckSavedJson();

  res_json.OnAwake();
  ign_off_count_ = 0;
  CheckSavedJson();
  EXPECT_TRUE(RemoveDirectory("./test_storage", true));
  EXPECT_TRUE(DeleteFile("./test_app_info.dat"));
  ::profile::Profile::instance()->config_file_name("smartDeviceLink.ini");
}

TEST_F(ResumptionDataJsonTest, Awake_AppNotSuspended) {
  ResumptionDataJson res_json;
  PrepareData();
  res_json.SaveApplication(app_mock);
  CheckSavedJson();

  res_json.OnAwake();
  ign_off_count_ = 0;
  CheckSavedJson();
}

TEST_F(ResumptionDataJsonTest, TwiceAwake_AppNotSuspended) {
  ResumptionDataJson res_json;
  PrepareData();
  res_json.SaveApplication(app_mock);
  CheckSavedJson();

  res_json.OnSuspend();
  suspend_count_++;
  res_json.OnAwake();
  ign_off_count_ = 0;
  CheckSavedJson();

  res_json.OnAwake();
  CheckSavedJson();
}

TEST_F(ResumptionDataJsonTest, GetHashId) {
  ResumptionDataJson res_json;
  PrepareData();
  res_json.SaveApplication(app_mock);
  CheckSavedJson();

  std::string test_hash;
  EXPECT_TRUE(res_json.GetHashId(policy_app_id_, "12345", test_hash));
}

TEST_F(ResumptionDataJsonTest, GetIgnOffTime_AfterSuspendAndAwake) {
  ResumptionDataJson res_json;
  uint32_t last_ign_off_time;
  PrepareData();
  res_json.SaveApplication(app_mock);
  CheckSavedJson();
  last_ign_off_time = res_json.GetIgnOffTime();
  EXPECT_NE(0, last_ign_off_time);
  ::profile::Profile::instance()->config_file_name("smartDeviceLink_test.ini");
  res_json.OnSuspend();

  uint32_t after_suspend;
  after_suspend = res_json.GetIgnOffTime();
  EXPECT_GE(last_ign_off_time, after_suspend);

  uint32_t after_awake;
  res_json.OnAwake();

  after_awake = res_json.GetIgnOffTime();
  EXPECT_GE(after_suspend, after_awake);

  EXPECT_TRUE(RemoveDirectory("./test_storage", true));
  EXPECT_TRUE(DeleteFile("./test_app_info.dat"));
  ::profile::Profile::instance()->config_file_name("smartDeviceLink.ini");
}

}  // namespace resumption_test
}  // namespace components
}  // namespace test
