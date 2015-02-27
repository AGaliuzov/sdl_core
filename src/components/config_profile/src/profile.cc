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

#include "config_profile/profile.h"

#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sstream>
#include <algorithm>

#include "config_profile/ini_file.h"
#include "utils/logger.h"
#include "utils/threads/thread.h"
#include "utils/file_system.h"
#ifdef ENABLE_SECURITY
#include <openssl/ssl.h>
#endif  // ENABLE_SECURITY

namespace {
#define LOG_UPDATED_VALUE(value, key, section) {\
  LOG4CXX_INFO(logger_, "Setting value '" << value\
  << "' for key '" << key\
  << "' in section '" << section << "'.");\
}

#define LOG_UPDATED_BOOL_VALUE(value, key, section) {\
  LOG4CXX_INFO(logger_, "Setting value '" << std::boolalpha << value\
  << "' for key '" << key\
  << "' in section '" << section << "'.");\
}

const char* kDefaultConfigFileName = "smartDeviceLink.ini";

const char* kMainSection = "MAIN";
#ifdef ENABLE_SECURITY
const char* kSecuritySection = "Security Manager";
#endif
const char* kPolicySection = "Policy";
const char* kHmiSection = "HMI";
const char* kAppInfoSection = "AppInfo";
const char* kMediaManagerSection = "MEDIA MANAGER";
const char* kGlobalPropertiesSection = "GLOBAL PROPERTIES";
const char* kVrCommandsSection = "VR COMMANDS";
const char* kTransportManagerSection = "TransportManager";
const char* kApplicationManagerSection = "ApplicationManager";
const char* kFilesystemRestrictionsSection = "FILESYSTEM RESTRICTIONS";
const char* kIAPSection = "IAP";
const char* kProtocolHandlerSection = "ProtocolHandler";
const char* kSDL4Section = "SDL4";
const char* kResumptionSection = "Resumption";

const char* kHmiCapabilitiesKey = "HMICapabilities";
const char* kPathToSnapshotKey = "PathToSnapshot";
const char* kPreloadedPTKey = "PreloadedPT";
const char* kAttemptsToOpenPolicyDBKey = "AttemptsToOpenPolicyDB";
const char* kOpenAttemptTimeoutMsKey = "OpenAttemptTimeoutMs";
const char* kServerAddressKey = "ServerAddress";
const char* kAppInfoStorageKey = "AppInfoStorage";
const char* kAppStorageFolderKey = "AppStorageFolder";
const char* kAppResourseFolderKey = "AppResourceFolder";
const char* kAppConfigFolderKey = "AppConfigFolder";
const char* kEnableProtocol4Key = "EnableProtocol4";
const char* kAppIconsFolderKey = "AppIconsFolder";
const char* kAppIconsFolderMaxSizeKey = "AppIconsFolderMaxSize";
const char* kAppIconsAmountToRemoveKey = "AppIconsAmountToRemove";
const char* kLaunchHMIKey = "LaunchHMI";
const char* kStartStreamRetry = "StartStreamRetry";
const char* kEnableRedecodingKey = "EnableRedecoding";
const char* kVideoStreamConsumerKey = "VideoStreamConsumer";
const char* kAudioStreamConsumerKey = "AudioStreamConsumer";
const char* kNamedVideoPipePathKey = "NamedVideoPipePath";
const char* kNamedAudioPipePathKey = "NamedAudioPipePath";
const char* kVideoStreamFileKey = "VideoStreamFile";
const char* kAudioStreamFileKey = "AudioStreamFile";

#ifdef ENABLE_SECURITY
const char* kSecurityProtocolKey = "Protocol";
const char* kSecurityCertificatePathKey = "CertificatePath";
const char* kSecuritySSLModeKey = "SSLMode";
const char* kSecurityKeyPathKey = "KeyPath";
const char* kSecurityCipherListKey = "CipherList";
const char* kSecurityVerifyPeerKey = "VerifyPeer";
#endif

#ifdef CUSTOMER_PASA
const char* kHMIHeartBeatTimeoutKey = "HMIHeartBeatTimeout";
const char* kLoggerSection = "LOGGING";
const char* kAudioMQPath = "MQAudioPath";
const char* kLoggerConfigFileKey = "LoggerConfigFile";
const char* kRemoteLoggingFlagFileKey = "RemoteLoggingFlagFile";
const char* kRemoteLoggingFlagFilePathKey = "RemoteLoggingFlagFilePath";
const char* kTargetLogFileHomeDirKey = "TargetLogFileHomeDir";
const char* kTargetLogFileNamePatternKey = "TargetLogFileNamePattern";
const char* kTargetBootCountFileKey = "TargetBootCountFile";
const char* kTargetTmpDirKey = "TargetTmpDir";
const char* kLogFileMaxSizeKey = "LogFileMaxSize";
#endif
const char* kAudioDataStoppedTimeoutKey = "AudioDataStoppedTimeout";
const char* kVideoDataStoppedTimeoutKey = "VideoDataStoppedTimeout";

const char* kMixingAudioSupportedKey = "MixingAudioSupported";
const char* kHelpPromptKey = "HelpPromt";
const char* kTimeoutPromptKey = "TimeOutPromt";
const char* kHelpTitleKey = "HelpTitle";
const char* kHelpCommandKey = "HelpCommand";
const char* kSystemFilesPathKey = "SystemFilesPath";
const char* kHeartBeatTimeoutKey = "HeartBeatTimeout";
const char* kUseLastStateKey = "UseLastState";
const char* kTCPAdapterPortKey = "TCPAdapterPort";
const char* kServerPortKey = "ServerPort";
const char* kVideoStreamingPortKey = "VideoStreamingPort";
const char* kAudioStreamingPortKey = "AudioStreamingPort";
const char* kStopStreamingTimeout = "StopStreamingTimeout";
const char* kTimeTestingPortKey = "TimeTestingPort";
const char* kThreadStackSizeKey = "ThreadStackSize";
const char* kMaxCmdIdKey = "MaxCmdID";
const char* kPutFileRequestKey = "PutFileRequest";
const char* kDeleteFileRequestKey = "DeleteFileRequest";
const char* kListFilesRequestKey = "ListFilesRequest";
const char* kDefaultTimeoutKey = "DefaultTimeout";
const char* kAppResumingTimeoutKey = "ApplicationResumingTimeout";
const char* kAppSavePersistentDataTimeoutKey = "AppSavePersistentDataTimeout";
const char* kResumptionDelayBeforeIgnKey = "ResumptionDelayBeforeIgn";
const char* kResumptionDelayAfterIgnKey = "ResumptionDelayAfterIgn";
const char* kAppDirectoryQuotaKey = "AppDirectoryQuota";
const char* kAppTimeScaleMaxRequestsKey = "AppTimeScaleMaxRequests";
const char* kAppRequestsTimeScaleKey = "AppRequestsTimeScale";
const char* kAppHmiLevelNoneTimeScaleMaxRequestsKey =
    "AppHMILevelNoneTimeScaleMaxRequests";
const char* kAppHmiLevelNoneRequestsTimeScaleKey =
    "AppHMILevelNoneRequestsTimeScale";
const char* kPendingRequestsAmoundKey = "PendingRequestsAmount";
const char* kSupportedDiagModesKey = "SupportedDiagModes";
const char* kTransportManagerDisconnectTimeoutKey = "DisconnectTimeout";
const char* kTTSDelimiterKey = "TTSDelimiter";
const char* kRecordingFileNameKey = "RecordingFileName";
const char* kRecordingFileSourceKey = "RecordingFileSource";
const char* kEnablePolicy = "EnablePolicy";
const char* kMmeDatabaseNameKey = "MMEDatabase";
const char* kEventMQKey = "EventMQ";
const char* kAckMQKey = "AckMQ";
const char* kApplicationListUpdateTimeoutKey = "ApplicationListUpdateTimeout";
const char* kReadDIDFrequencykey = "ReadDIDRequest";
const char* kGetVehicleDataFrequencyKey = "GetVehicleDataRequest";
const char* kLegacyProtocolMaskKey = "LegacyProtocol";
const char* kHubProtocolMaskKey = "HubProtocol";
const char* kPoolProtocolMaskKey = "PoolProtocol";
const char* kIAPSystemConfigKey = "IAPSystemConfig";
const char* kIAP2SystemConfigKey = "IAP2SystemConfig";
const char* kIAP2HubConnectAttemptskey = "IAP2HubConnectAttempts";
const char* kIAPHubConnectionWaitTimeoutKey = "ConnectionWaitTimeout";
const char* kIAPArmEventTimeoutKey = "ArmEventTimeout";
const char* kDefaultHubProtocolIndexKey = "DefaultHubProtocolIndex";
const char* kTTSGlobalPropertiesTimeoutKey = "TTSGlobalPropertiesTimeout";
const char* kMaximumPayloadSizeKey = "MaximumPayloadSize";
const char* kFrequencyCount = "FrequencyCount";
const char* kFrequencyTime = "FrequencyTime";
const char* kHashStringSizeKey = "HashStringSize";

const char* kDefaultPoliciesSnapshotFileName = "sdl_snapshot.json";
const char* kDefaultHmiCapabilitiesFileName = "hmi_capabilities.json";
const char* kDefaultPreloadedPTFileName = "sdl_preloaded_pt.json";
const char* kDefaultServerAddress = "127.0.0.1";
const char* kDefaultAppInfoFileName = "app_info.dat";
const char* kDefaultSystemFilesPath = "/tmp/fs/mp/images/ivsu_cache";
const char* kDefaultTtsDelimiter = ",";
#ifdef CUSTOMER_PASA
const uint32_t kDefaultHMIHeartBeatTimeout = 3;  // timeout in seconds
const char* kDefaultMQName = "/dev/mqueue/AppLinkAudioPass";
const char* kDefaultLog4cxxConfig = "/fs/mp/etc/AppLink/log4cxx.properties";
const char* kDefaultRemoteLoggingFlagFile = "log/capturelog.evt";
const char* kDefaultRemoteLoggingFlagFilePath = "/fs/usb0/";
const char* kDefaultTargetLogFileHomeDir = "/fs/rwdata/logs/";
const char* kDefaultTargetLogFileNamePattern ="smartdevicelink.log";
const char* kDefaultTargetBootCountFile = "/fs/rwdata/.flags/boot_count";
const char* kDefaultTargetTmpDir = "/fs/tmpfs";
const char* kDefaultLogFileMaxSize = "1024K";
#endif
const uint32_t kDefaultAudioDataStoppedTimeout = 1000;
const uint32_t kDefaultVideoDataStoppedTimeout = 1000;
const char* kDefaultMmeDatabaseName = "/dev/qdb/mediaservice_db";
const char* kDefaultEventMQ = "/dev/mqueue/ToSDLCoreUSBAdapter";
const char* kDefaultAckMQ = "/dev/mqueue/FromSDLCoreUSBAdapter";
const char* kDefaultRecordingFileSourceName = "audio.8bit.wav";
const char* kDefaultRecordingFileName = "record.wav";
const char* kDefaultThreadPoolSize = "ThreadPoolSize";
const char* kDefaultLegacyProtocolMask = "com.ford.sync.prot";
const char* kDefaultHubProtocolMask = "com.smartdevicelink.prot";
const char* kDefaultPoolProtocolMask = "com.smartdevicelink.prot";
const char* kDefaultIAPSystemConfig = "/fs/mp/etc/mm/ipod.cfg";
const char* kDefaultIAP2SystemConfig = "/fs/mp/etc/mm/iap2.cfg";

#ifdef ENABLE_SECURITY
const char* kDefaultSecurityProtocol = "TLSv1.2";
const char* kDefaultSSLMode = "CLIENT";
const bool kDefaultVerifyPeer = false;
#endif // ENABLE_SECURITY

const uint32_t kDefaultHubProtocolIndex = 0;
const uint32_t kDefaultHeartBeatTimeout = 0;
const uint16_t kDefautTransportManagerTCPPort = 12345;
const uint16_t kDefaultServerPort = 8087;
const uint16_t kDefaultVideoStreamingPort = 5050;
const uint16_t kDefaultAudioStreamingPort = 5080;
const uint32_t kDefaultStopStreamingTimeout = 1;
const uint16_t kDefaultTimeTestingPort = 5090;
const uint32_t kDefaultMaxCmdId = 2000000000;
const uint32_t kDefaultPutFileRequestInNone = 5;
const uint32_t kDefaultDeleteFileRequestInNone = 5;
const uint32_t kDefaultListFilesRequestInNone = 5;
const uint32_t kDefaultTimeout = 10;
const uint32_t kDefaultAppResumingTimeout = 3;
const uint32_t kDefaultAppSavePersistentDataTimeout = 10;
const uint32_t kDefaultResumptionDelayBeforeIgn = 30;
const uint32_t kDefaultResumptionDelayAfterIgn = 30;
const uint32_t kDefaultHashStringSize = 32;

const uint32_t kDefaultDirQuota = 104857600;
const uint32_t kDefaultAppTimeScaleMaxRequests = 0;
const uint32_t kDefaultAppRequestsTimeScale = 0;
const uint32_t kDefaultAppHmiLevelNoneTimeScaleMaxRequests = 100;
const uint32_t kDefaultAppHmiLevelNoneRequestsTimeScale = 10;
const uint32_t kDefaultPendingRequestsAmount = 0;
const uint32_t kDefaultTransportManagerDisconnectTimeout = 0;
const uint32_t kDefaultApplicationListUpdateTimeout = 1;
const std::pair<uint32_t, uint32_t> kReadDIDFrequency = { 5, 1 };
const std::pair<uint32_t, uint32_t> kGetVehicleDataFrequency = { 5, 1 };
const std::pair<uint32_t, uint32_t> kStartStreamRetryAmount = { 3, 1 };
const uint32_t kDefaultMaxThreadPoolSize = 2;
const int kDefaultIAP2HubConnectAttempts = 0;
const int kDefaultIAPHubConnectionWaitTimeout = 10;
const int kDefaultIAPArmEventTimeout = 500;
const uint16_t kDefaultTTSGlobalPropertiesTimeout = 20;
// TCP MTU - header size = 1500 - 12
const size_t kDefaultMaximumPayloadSize = 1500 - 12;
const size_t kDefaultFrequencyCount = 1000;
const size_t kDefaultFrequencyTime = 1000;
const uint16_t kDefaultAttemptsToOpenPolicyDB = 5;
const uint16_t kDefaultOpenAttemptTimeoutMsKey = 500;
const uint32_t kDefaultAppIconsFolderMaxSize = 104857600;
const uint32_t kDefaultAppIconsAmountToRemove = 1;

}  // namespace

namespace profile {

CREATE_LOGGERPTR_GLOBAL(logger_, "Profile")

Profile::Profile()
    : launch_hmi_(true),
      app_config_folder_(),
      app_storage_folder_(),
      app_resourse_folder_(),
      enable_protocol_4_(false),
      app_icons_folder_(),
      app_icons_folder_max_size_(kDefaultAppIconsFolderMaxSize),
      app_icons_amount_to_remove_(kDefaultAppIconsAmountToRemove),
      config_file_name_(kDefaultConfigFileName),
      server_address_(kDefaultServerAddress),
      server_port_(kDefaultServerPort),
      video_streaming_port_(kDefaultVideoStreamingPort),
      audio_streaming_port_(kDefaultAudioStreamingPort),
      time_testing_port_(kDefaultTimeTestingPort),
      hmi_capabilities_file_name_(kDefaultHmiCapabilitiesFileName),
      help_prompt_(),
      time_out_promt_(),
      min_tread_stack_size_(threads::Thread::kMinStackSize),
      is_mixing_audio_supported_(false),
      is_redecoding_enabled_(false),
      max_cmd_id_(kDefaultMaxCmdId),
      default_timeout_(kDefaultTimeout),
      app_resuming_timeout_(kDefaultAppResumingTimeout),
      app_dir_quota_(kDefaultDirQuota),
      app_hmi_level_none_time_scale_max_requests_(
          kDefaultAppHmiLevelNoneTimeScaleMaxRequests),
      app_hmi_level_none_requests_time_scale_(
          kDefaultAppHmiLevelNoneRequestsTimeScale),
      app_time_scale_max_requests_(kDefaultAppTimeScaleMaxRequests),
      app_requests_time_scale_(kDefaultAppRequestsTimeScale),
      pending_requests_amount_(kDefaultPendingRequestsAmount),
      put_file_in_none_(kDefaultPutFileRequestInNone),
      delete_file_in_none_(kDefaultDeleteFileRequestInNone),
      list_files_in_none_(kDefaultListFilesRequestInNone),
      app_info_storage_(kDefaultAppInfoFileName),
      heart_beat_timeout_(kDefaultHeartBeatTimeout),
      policy_snapshot_file_name_(kDefaultPoliciesSnapshotFileName),
      enable_policy_(false),
      transport_manager_disconnect_timeout_(
          kDefaultTransportManagerDisconnectTimeout),
      use_last_state_(false),
      supported_diag_modes_(),
      system_files_path_(kDefaultSystemFilesPath),
      transport_manager_tcp_adapter_port_(kDefautTransportManagerTCPPort),
      tts_delimiter_(kDefaultTtsDelimiter),
#ifdef CUSTOMER_PASA
      hmi_heart_beat_timeout_(kDefaultHMIHeartBeatTimeout),
      audio_mq_path_(kDefaultMQName),
      log4cxx_config_file_(kDefaultLog4cxxConfig),
      remote_logging_flag_file_(kDefaultRemoteLoggingFlagFile),
      remote_logging_flag_file_path_(kDefaultRemoteLoggingFlagFilePath),
      target_log_file_home_dir_(kDefaultTargetLogFileHomeDir),
      target_log_file_name_pattern_(kDefaultTargetLogFileNamePattern),
      target_boot_count_file_(kDefaultTargetBootCountFile),
      target_tmp_dir_(kDefaultTargetTmpDir),
#endif
    audio_data_stopped_timeout_(kDefaultAudioDataStoppedTimeout),
    video_data_stopped_timeout_(kDefaultVideoDataStoppedTimeout),
    mme_db_name_(kDefaultMmeDatabaseName),
    event_mq_name_(kDefaultEventMQ),
    ack_mq_name_(kDefaultAckMQ),
    recording_file_source_(kDefaultRecordingFileSourceName),
    recording_file_name_(kDefaultRecordingFileName),
    application_list_update_timeout_(kDefaultApplicationListUpdateTimeout),
    iap_legacy_protocol_mask_(kDefaultLegacyProtocolMask),
    iap_hub_protocol_mask_(kDefaultHubProtocolMask),
    iap_pool_protocol_mask_(kDefaultPoolProtocolMask),
    iap_system_config_(kDefaultIAPSystemConfig),
    iap2_system_config_(kDefaultIAP2SystemConfig),
    iap2_hub_connect_attempts_(kDefaultIAP2HubConnectAttempts),
    iap_hub_connection_wait_timeout_(kDefaultIAPHubConnectionWaitTimeout),
    iap_arm_event_timeout_(kDefaultIAPArmEventTimeout),
    tts_global_properties_timeout_(kDefaultTTSGlobalPropertiesTimeout),
    attempts_to_open_policy_db_(kDefaultAttemptsToOpenPolicyDB),
    open_attempt_timeout_ms_(kDefaultAttemptsToOpenPolicyDB),
    hash_string_size_(kDefaultHashStringSize) {
}

Profile::~Profile() {
}

void Profile::config_file_name(const std::string& fileName) {
  if (false == fileName.empty()) {
    config_file_name_ = fileName;
    UpdateValues();
  }
}

const std::string& Profile::config_file_name() const {
  return config_file_name_;
}

bool Profile::launch_hmi() const {
  return launch_hmi_;
}

const std::string& Profile::app_config_folder() const {
  return app_config_folder_;
}

const std::string& Profile::app_storage_folder() const {
  return app_storage_folder_;
}

const std::string& Profile::app_resourse_folder() const {
  return app_resourse_folder_;
}

bool Profile::enable_protocol_4() const {
  return enable_protocol_4_;
}

const std::string&Profile::app_icons_folder() const {
  return app_icons_folder_;
}

const uint32_t&Profile::app_icons_folder_max_size() const {
  return app_icons_folder_max_size_;
}

const uint32_t&Profile::app_icons_amount_to_remove() const {
  return app_icons_amount_to_remove_;
}

const std::string& Profile::hmi_capabilities_file_name() const {
  return hmi_capabilities_file_name_;
}

const std::string& Profile::server_address() const {
  return server_address_;
}

const std::vector<std::string>& Profile::help_prompt() const {
  return help_prompt_;
}

const std::vector<std::string>& Profile::time_out_promt() const {
  return time_out_promt_;
}

const std::vector<std::string>& Profile::vr_commands() const {
  return vr_commands_;
}

const uint32_t& Profile::max_cmd_id() const {
  return max_cmd_id_;
}

const uint32_t& Profile::default_timeout() const {
  return default_timeout_;
}

const uint32_t& Profile::app_resuming_timeout() const {
  return app_resuming_timeout_;
}

const uint32_t& Profile::app_resumption_save_persistent_data_timeout() const {
  return app_resumption_save_persistent_data_timeout_;
}

const std::string& Profile::vr_help_title() const {
  return vr_help_title_;
}

const uint16_t& Profile::server_port() const {
  return server_port_;
}

const uint16_t& Profile::video_streaming_port() const {
  return video_streaming_port_;
}

const uint16_t& Profile::audio_streaming_port() const {
  return audio_streaming_port_;
}

uint32_t Profile::stop_streaming_timeout() const {
  return stop_streaming_timeout_;
}

const uint16_t& Profile::time_testing_port() const {
  return time_testing_port_;
}

const uint64_t& Profile::thread_min_stack_size() const {
  return min_tread_stack_size_;
}

bool Profile::is_mixing_audio_supported() const {
  return is_mixing_audio_supported_;
}

const uint32_t& Profile::app_dir_quota() const {
  return app_dir_quota_;
}

bool Profile::is_redecoding_enabled() const {
  return is_redecoding_enabled_;
}

const std::string& Profile::video_server_type() const {
  return video_consumer_type_;
}

const std::string& Profile::audio_server_type() const {
  return audio_consumer_type_;
}

const std::string& Profile::named_video_pipe_path() const {
  return named_video_pipe_path_;
}

const std::string& Profile::named_audio_pipe_path() const {
  return named_audio_pipe_path_;
}

const uint32_t& Profile::app_hmi_level_none_time_scale() const {
  return app_hmi_level_none_requests_time_scale_;
}

const uint32_t& Profile::app_hmi_level_none_time_scale_max_requests() const {
  return app_hmi_level_none_time_scale_max_requests_;
}

const std::string& Profile::video_stream_file() const {
  return video_stream_file_;
}

const std::string& Profile::audio_stream_file() const {
  return audio_stream_file_;
}

#ifdef CUSTOMER_PASA

const uint32_t& Profile::hmi_heart_beat_timeout() {
  return hmi_heart_beat_timeout_;
}

const std::string &profile::Profile::audio_mq_path() const {
  LOG4CXX_INFO(logger_, "Default MQ name " << audio_mq_path_);
  return audio_mq_path_;
}

const std::string& Profile::log4cxx_config_file() const {
  return log4cxx_config_file_;
}
const std::string& Profile::remote_logging_flag_file() const {
  return remote_logging_flag_file_;
}

const std::string& Profile::remote_logging_flag_file_path() const {
  return remote_logging_flag_file_path_;
}

const std::string& Profile::target_log_file_home_dir() const {
  return target_log_file_home_dir_;
}

const std::string& Profile::target_log_file_name_pattern() const {
  return target_log_file_name_pattern_;
}

const std::string& Profile::target_boot_count_file() const {
  return target_boot_count_file_;
}

const std::string& Profile::target_tmp_dir() const {
  return target_tmp_dir_;
}

const std::string& Profile::log_file_max_size() const {
  return log_file_max_size_;
}

#endif

const std::uint32_t Profile::audio_data_stopped_timeout() const {
  return audio_data_stopped_timeout_;
}

const std::uint32_t Profile::video_data_stopped_timeout() const {
  return video_data_stopped_timeout_;
}

const uint32_t& Profile::app_time_scale() const {
  return app_requests_time_scale_;
}

const uint32_t& Profile::app_time_scale_max_requests() const {
  return app_time_scale_max_requests_;
}

const uint32_t& Profile::pending_requests_amount() const {
  return pending_requests_amount_;
}

const uint32_t& Profile::put_file_in_none() const {
  return put_file_in_none_;
}

const uint32_t& Profile::delete_file_in_none() const {
  return delete_file_in_none_;
}

const uint32_t& Profile::list_files_in_none() const {
  return list_files_in_none_;
}

const std::string& Profile::app_info_storage() const {
  return app_info_storage_;
}

int32_t Profile::heart_beat_timeout() const {
  return heart_beat_timeout_;
}

const std::string& Profile::preloaded_pt_file() const {
  return preloaded_pt_file_;
}

const std::string& Profile::policies_snapshot_file_name() const {
  return policy_snapshot_file_name_;
}

bool Profile::enable_policy() const {
  return enable_policy_;
}

uint32_t Profile::transport_manager_disconnect_timeout() const {
  return transport_manager_disconnect_timeout_;
}

bool Profile::use_last_state() const {
  return use_last_state_;
}

const std::string& Profile::system_files_path() const {
  return system_files_path_;
}

const std::vector<uint32_t>& Profile::supported_diag_modes() const {
  return supported_diag_modes_;
}

uint16_t Profile::transport_manager_tcp_adapter_port() const {
  return transport_manager_tcp_adapter_port_;
}

const std::string& Profile::tts_delimiter() const {
  return tts_delimiter_;
}

const std::string& Profile::recording_file_source() const {
  return recording_file_source_;
}

const std::string&Profile::recording_file_name() const {
  return recording_file_name_;
}

const std::string& Profile::mme_db_name() const {
  return mme_db_name_;
}

const std::string& Profile::event_mq_name() const {
  return event_mq_name_;
}

const std::string& Profile::ack_mq_name() const {
  return ack_mq_name_;
}

uint32_t Profile::application_list_update_timeout() const {
  return application_list_update_timeout_;
}

const std::pair<uint32_t, int32_t>& Profile::read_did_frequency() const {
  return read_did_frequency_;
}

const std::pair<uint32_t, int32_t>& Profile::get_vehicle_data_frequency() const {
  return get_vehicle_data_frequency_;
}

const std::pair<uint32_t, int32_t>& Profile::start_stream_retry_amount() const {
  return start_stream_retry_amount_;
}

uint32_t Profile::thread_pool_size() const {
  return max_thread_pool_size_;
}

uint32_t Profile::default_hub_protocol_index() const {
  return default_hub_protocol_index_;
}

const std::string& Profile::iap_legacy_protocol_mask() const {
  return iap_legacy_protocol_mask_;
}

const std::string& Profile::iap_hub_protocol_mask() const {
  return iap_hub_protocol_mask_;
}

const std::string& Profile::iap_pool_protocol_mask() const {
  return iap_pool_protocol_mask_;
}

const std::string& Profile::iap_system_config() const {
  return iap_system_config_;
}

const std::string& Profile::iap2_system_config() const {
  return iap2_system_config_;
}

int Profile::iap2_hub_connect_attempts() const {
  return iap2_hub_connect_attempts_;
}

int Profile::iap_hub_connection_wait_timeout() const {
  return iap_hub_connection_wait_timeout_;
}

int Profile::iap_arm_event_timeout() const {
  return iap_arm_event_timeout_;
}

size_t Profile::maximum_payload_size() const {
  size_t maximum_payload_size = 0;
  ReadUIntValue(&maximum_payload_size, kDefaultMaximumPayloadSize,
                kProtocolHandlerSection, kMaximumPayloadSizeKey);
  return maximum_payload_size;
}

size_t Profile::message_frequency_count() const {
  size_t message_frequency_count = 0;
  ReadUIntValue(&message_frequency_count, kDefaultFrequencyCount,
                kProtocolHandlerSection, kFrequencyCount);
  return message_frequency_count;
}

size_t Profile::message_frequency_time() const {
  size_t message_frequency_time = 0;
  ReadUIntValue(&message_frequency_time, kDefaultFrequencyTime,
                kProtocolHandlerSection, kFrequencyTime);
  return message_frequency_time;
}

uint16_t Profile::attempts_to_open_policy_db() const {
  return attempts_to_open_policy_db_;
}

uint16_t Profile::open_attempt_timeout_ms() const {
  return open_attempt_timeout_ms_;
}

uint32_t Profile::resumption_delay_before_ign() const {
  return resumption_delay_before_ign_;
}

uint32_t Profile::resumption_delay_after_ign() const {
  return resumption_delay_after_ign_;
}

uint32_t Profile::hash_string_size() const {
  return hash_string_size_;
}

uint16_t Profile::tts_global_properties_timeout() const {
  return tts_global_properties_timeout_;
}

#ifdef ENABLE_SECURITY

const std::string& Profile::cert_path() const {
  return cert_path_;
}

const std::string& Profile::ssl_mode() const {
  return ssl_mode_;
}

const std::string& Profile::key_path() const {
  return key_path_;
}

const std::string& Profile::ciphers_list() const {
  return ciphers_list_;
}

bool Profile::verify_peer() const {
  return verify_peer_;
}

const std::string& Profile::security_manager_protocol_name() const {
  return security_manager_protocol_name_;
}

#endif // ENABLE_SECURITY

void Profile::UpdateValues() {
  LOG4CXX_AUTO_TRACE(logger_);

  // Launch HMI parameter
  ReadBoolValue(&launch_hmi_, true, kHmiSection, kLaunchHMIKey);

  LOG_UPDATED_BOOL_VALUE(launch_hmi_, kLaunchHMIKey, kHmiSection);

#ifdef ENABLE_SECURITY

  ReadStringValue(&security_manager_protocol_name_, kDefaultSecurityProtocol, kSecuritySection,
      kSecurityProtocolKey);

  ReadStringValue(&cert_path_, "", kSecuritySection,
      kSecurityCertificatePathKey);

  ReadStringValue(&ssl_mode_, kDefaultSSLMode, kSecuritySection,
      kSecuritySSLModeKey);

  ReadStringValue(&key_path_, "", kSecuritySection, kSecurityKeyPathKey);

  ReadStringValue(&ciphers_list_, SSL_TXT_ALL, kSecuritySection,
      kSecurityCipherListKey);

  ReadBoolValue(&verify_peer_, kDefaultVerifyPeer, kSecuritySection,
      kSecurityVerifyPeerKey);

#endif  // ENABLE_SECURITY

  // Application config folder
  ReadStringValue(&app_config_folder_,
                  file_system::CurrentWorkingDirectory().c_str(), kMainSection,
                  kAppConfigFolderKey);

  LOG_UPDATED_VALUE(app_config_folder_, kAppConfigFolderKey, kMainSection);

  // Application storage folder
  ReadStringValue(&app_storage_folder_,
                  file_system::CurrentWorkingDirectory().c_str(), kMainSection,
                  kAppStorageFolderKey);

  LOG_UPDATED_VALUE(app_storage_folder_, kAppStorageFolderKey, kMainSection);

  // Application resourse folder
  ReadStringValue(&app_resourse_folder_,
                  file_system::CurrentWorkingDirectory().c_str(), kMainSection,
                  kAppResourseFolderKey);

  LOG_UPDATED_VALUE(app_resourse_folder_, kAppResourseFolderKey, kMainSection);

  // Enable protocol ver.4 parameter
  ReadBoolValue(&enable_protocol_4_, false, kSDL4Section, kEnableProtocol4Key);

  LOG_UPDATED_BOOL_VALUE(enable_protocol_4_, kEnableProtocol4Key, kSDL4Section);

  // Application icon folder
  ReadStringValue(&app_icons_folder_,
                  file_system::CurrentWorkingDirectory().c_str(), kSDL4Section,
                  kAppIconsFolderKey);

  LOG_UPDATED_VALUE(app_icons_folder_, kAppIconsFolderKey, kSDL4Section);

  // Application icon folder maximum size
  ReadUIntValue(&app_icons_folder_max_size_, kDefaultAppIconsFolderMaxSize,
                kSDL4Section, kAppIconsFolderMaxSizeKey);

  LOG_UPDATED_VALUE(app_icons_folder_max_size_, kAppIconsFolderMaxSizeKey,
                    kSDL4Section);

  // Application icon folder maximum size
  ReadUIntValue(&app_icons_amount_to_remove_, kDefaultAppIconsAmountToRemove,
                kSDL4Section, kAppIconsAmountToRemoveKey);

  LOG_UPDATED_VALUE(app_icons_amount_to_remove_, kAppIconsAmountToRemoveKey,
                    kSDL4Section);

  // Application info file name
  ReadStringValue(&app_info_storage_, kDefaultAppInfoFileName, kAppInfoSection,
                  kAppInfoStorageKey);

#ifdef CUSTOMER_PASA
  app_info_storage_ = app_storage_folder_ + "/" + app_info_storage_;
#endif
  LOG_UPDATED_VALUE(app_info_storage_, kAppInfoStorageKey, kAppInfoSection);

  // Server address
  ReadStringValue(&server_address_, kDefaultServerAddress, kHmiSection,
                  kServerAddressKey);

  LOG_UPDATED_VALUE(server_address_, kServerAddressKey, kHmiSection);

  // HMI capabilities
  ReadStringValue(&hmi_capabilities_file_name_, kDefaultHmiCapabilitiesFileName,
                  kMainSection, kHmiCapabilitiesKey);

  hmi_capabilities_file_name_ = app_config_folder_ + "/"
      + hmi_capabilities_file_name_;

  LOG_UPDATED_VALUE(hmi_capabilities_file_name_, kHmiCapabilitiesKey,
                    kMainSection);

  // Server port
  ReadUIntValue(&server_port_, kDefaultServerPort, kHmiSection, kServerPortKey);

  LOG_UPDATED_VALUE(server_port_, kServerPortKey, kHmiSection);

  // Video streaming port
  ReadUIntValue(&video_streaming_port_, kDefaultVideoStreamingPort, kHmiSection,
                kVideoStreamingPortKey);

  LOG_UPDATED_VALUE(video_streaming_port_, kVideoStreamingPortKey, kHmiSection);
  // Audio streaming port
  ReadUIntValue(&audio_streaming_port_, kDefaultAudioStreamingPort, kHmiSection,
                kAudioStreamingPortKey);

  LOG_UPDATED_VALUE(audio_streaming_port_, kAudioStreamingPortKey, kHmiSection);

  // Streaming timeout
  ReadUIntValue(&stop_streaming_timeout_, kDefaultStopStreamingTimeout,
                kHmiSection, kStopStreamingTimeout);

  stop_streaming_timeout_ = std::max(kDefaultStopStreamingTimeout,
                                     stop_streaming_timeout_);

  LOG_UPDATED_VALUE(stop_streaming_timeout_, kStopStreamingTimeout, kHmiSection);

  // Time testing port
  ReadUIntValue(&time_testing_port_, kDefaultTimeTestingPort, kMainSection,
                kTimeTestingPortKey);

  LOG_UPDATED_VALUE(time_testing_port_, kTimeTestingPortKey, kMainSection);

  // Minimum thread stack size
  ReadUIntValue(&min_tread_stack_size_, threads::Thread::kMinStackSize,
                kMainSection, kThreadStackSizeKey);

  if (min_tread_stack_size_ < threads::Thread::kMinStackSize) {
    min_tread_stack_size_ = threads::Thread::kMinStackSize;
  }

  LOG_UPDATED_VALUE(min_tread_stack_size_, kThreadStackSizeKey, kMainSection);

  // Start stream retry frequency
  ReadUintIntPairValue(&start_stream_retry_amount_, kStartStreamRetryAmount,
                       kMediaManagerSection, kStartStreamRetry);

  // Redecoding parameter
  ReadBoolValue(&is_redecoding_enabled_, false, kMediaManagerSection,
                kEnableRedecodingKey);

  LOG_UPDATED_BOOL_VALUE(is_redecoding_enabled_, kEnableRedecodingKey,
                         kMediaManagerSection);

  // Video consumer type
  ReadStringValue(&video_consumer_type_, "", kMediaManagerSection,
                  kVideoStreamConsumerKey);

  LOG_UPDATED_VALUE(video_consumer_type_, kVideoStreamConsumerKey,
                    kMediaManagerSection);

  // Audio stream consumer
  ReadStringValue(&audio_consumer_type_, "", kMediaManagerSection,
                  kAudioStreamConsumerKey);

  LOG_UPDATED_VALUE(audio_consumer_type_, kAudioStreamConsumerKey,
                    kMediaManagerSection);

  // Named video pipe path
  ReadStringValue(&named_video_pipe_path_, "", kMediaManagerSection,
                  kNamedVideoPipePathKey);

  named_video_pipe_path_ = app_storage_folder_ + "/" + named_video_pipe_path_;

  LOG_UPDATED_VALUE(named_video_pipe_path_, kNamedVideoPipePathKey,
                    kMediaManagerSection);

  // Named audio pipe path
  ReadStringValue(&named_audio_pipe_path_, "", kMediaManagerSection,
                  kNamedAudioPipePathKey);

  named_audio_pipe_path_ = app_storage_folder_ + "/" + named_audio_pipe_path_;

  LOG_UPDATED_VALUE(named_audio_pipe_path_, kNamedAudioPipePathKey,
                    kMediaManagerSection);

  // Video stream file
  ReadStringValue(&video_stream_file_, "", kMediaManagerSection,
                  kVideoStreamFileKey);

  video_stream_file_ = app_storage_folder_ + "/" + video_stream_file_;

  LOG_UPDATED_VALUE(video_stream_file_, kVideoStreamFileKey,
                    kMediaManagerSection);

  // Audio stream file
  ReadStringValue(&audio_stream_file_, "", kMediaManagerSection,
                  kAudioStreamFileKey);

  audio_stream_file_ = app_storage_folder_ + "/" + audio_stream_file_;

  LOG_UPDATED_VALUE(audio_stream_file_, kAudioStreamFileKey,
                    kMediaManagerSection);

#ifdef CUSTOMER_PASA
  // Heartbeat timeout
  ReadUIntValue(&hmi_heart_beat_timeout_, kDefaultHeartBeatTimeout, kMainSection,
      kHMIHeartBeatTimeoutKey);

  LOG_UPDATED_VALUE(hmi_heart_beat_timeout_, kHMIHeartBeatTimeoutKey, kMainSection);

  ReadStringValue(&audio_mq_path_, "", kMediaManagerSection,
      kAudioMQPath);

  LOG_UPDATED_VALUE(log4cxx_config_file_, kAudioMQPath,
      kMediaManagerSection);
  // log4cxx config file
  ReadStringValue(&log4cxx_config_file_, "", kLoggerSection,
      kLoggerConfigFileKey);

  LOG_UPDATED_VALUE(log4cxx_config_file_, kLoggerConfigFileKey,
      kLoggerSection);
  // Remote logging flag file
  ReadStringValue(&remote_logging_flag_file_, "", kLoggerSection,
      kRemoteLoggingFlagFileKey);

  LOG_UPDATED_VALUE(remote_logging_flag_file_, kRemoteLoggingFlagFileKey,
      kLoggerSection);

  // Remote logging flag file
  ReadStringValue(&remote_logging_flag_file_path_, "", kLoggerSection,
      kRemoteLoggingFlagFilePathKey);

  LOG_UPDATED_VALUE(remote_logging_flag_file_path_, kRemoteLoggingFlagFilePathKey,
      kLoggerSection);

  ReadStringValue(&target_log_file_home_dir_, "", kLoggerSection,
      kTargetLogFileHomeDirKey);

  LOG_UPDATED_VALUE(target_log_file_home_dir_, kTargetLogFileHomeDirKey,
      kLoggerSection);

  ReadStringValue(&target_log_file_name_pattern_, "", kLoggerSection,
      kTargetLogFileNamePatternKey);

  LOG_UPDATED_VALUE(target_log_file_name_pattern_, kTargetLogFileNamePatternKey,
      kLoggerSection);

  ReadStringValue(&target_boot_count_file_, "", kMainSection,
      kTargetBootCountFileKey);

  LOG_UPDATED_VALUE(target_boot_count_file_, kTargetBootCountFileKey,
      kMainSection);

  ReadStringValue(&target_tmp_dir_, "", kMainSection,
      kTargetTmpDirKey);

  LOG_UPDATED_VALUE(target_tmp_dir_, kTargetTmpDirKey,
      kMainSection);

  ReadStringValue(&log_file_max_size_, kDefaultLogFileMaxSize, kLoggerSection,
      kLogFileMaxSizeKey);

  LOG_UPDATED_VALUE(log_file_max_size_, kLogFileMaxSizeKey, kLoggerSection);


#endif
  ReadUIntValue(&audio_data_stopped_timeout_, kDefaultAudioDataStoppedTimeout,
                kMediaManagerSection, kAudioDataStoppedTimeoutKey);

  LOG_UPDATED_VALUE(audio_data_stopped_timeout_, kAudioDataStoppedTimeoutKey,
                    kMediaManagerSection);

  ReadUIntValue(&video_data_stopped_timeout_, kDefaultVideoDataStoppedTimeout,
                kMediaManagerSection, kVideoDataStoppedTimeoutKey);

  LOG_UPDATED_VALUE(video_data_stopped_timeout_, kVideoDataStoppedTimeoutKey,
                    kMediaManagerSection);

  // Mixing audio parameter
  ReadBoolValue(&is_mixing_audio_supported_, false, kMainSection,
                kMixingAudioSupportedKey);

  LOG_UPDATED_BOOL_VALUE(is_mixing_audio_supported_, kMixingAudioSupportedKey,
                         kMainSection);

  // Maximum command id value
  ReadUIntValue(&max_cmd_id_, kDefaultMaxCmdId, kMainSection, kMaxCmdIdKey);

  if (max_cmd_id_ < 0) {
    max_cmd_id_ = kDefaultMaxCmdId;
  }

  LOG_UPDATED_VALUE(max_cmd_id_, kMaxCmdIdKey, kMainSection);

  // PutFile restrictions
  ReadUIntValue(&put_file_in_none_, kDefaultPutFileRequestInNone,
                kFilesystemRestrictionsSection, kPutFileRequestKey);

  if (put_file_in_none_ < 0) {
    put_file_in_none_ = kDefaultPutFileRequestInNone;
  }

  LOG_UPDATED_VALUE(put_file_in_none_, kPutFileRequestKey,
                    kFilesystemRestrictionsSection);

  // DeleteFileRestrictions
  ReadUIntValue(&delete_file_in_none_, kDefaultDeleteFileRequestInNone,
                kFilesystemRestrictionsSection, kDeleteFileRequestKey);

  if (delete_file_in_none_ < 0) {
    delete_file_in_none_ = kDefaultDeleteFileRequestInNone;
  }

  LOG_UPDATED_VALUE(delete_file_in_none_, kDeleteFileRequestKey,
                    kFilesystemRestrictionsSection);

  // ListFiles restrictions
  ReadUIntValue(&list_files_in_none_, kDefaultListFilesRequestInNone,
                kFilesystemRestrictionsSection, kListFilesRequestKey);

  if (list_files_in_none_ < 0) {
    list_files_in_none_ = kDefaultListFilesRequestInNone;
  }

  LOG_UPDATED_VALUE(list_files_in_none_, kListFilesRequestKey,
                    kFilesystemRestrictionsSection);

  // Default timeout
  ReadUIntValue(&default_timeout_, kDefaultTimeout, kMainSection,
                kDefaultTimeoutKey);

  if (default_timeout_ <= 0) {
    default_timeout_ = kDefaultTimeout;
  }

  LOG_UPDATED_VALUE(default_timeout_, kDefaultTimeoutKey, kMainSection);

  // Application resuming timeout
  ReadUIntValue(&app_resuming_timeout_, kDefaultAppResumingTimeout,
                kResumptionSection, kAppResumingTimeoutKey);

  if (app_resuming_timeout_ <= 0) {
    app_resuming_timeout_ = kDefaultAppResumingTimeout;
  }
  // Save resumption info to File System
  LOG_UPDATED_VALUE(app_resuming_timeout_, kAppSavePersistentDataTimeoutKey,
                    kResumptionSection);

  ReadUIntValue(&app_resumption_save_persistent_data_timeout_,
                kDefaultAppSavePersistentDataTimeout, kResumptionSection,
                kAppSavePersistentDataTimeoutKey);
  if (app_resuming_timeout_ <= 0) {
    app_resuming_timeout_ = kDefaultAppSavePersistentDataTimeout;
  }

  LOG_UPDATED_VALUE(app_resuming_timeout_, kAppResumingTimeoutKey,
                    kResumptionSection);
  // Open attempt timeout in ms
  ReadUIntValue(&resumption_delay_before_ign_, kDefaultResumptionDelayBeforeIgn,
                kResumptionSection, kResumptionDelayBeforeIgnKey);

  LOG_UPDATED_VALUE(resumption_delay_after_ign_, kResumptionDelayBeforeIgnKey,
                    kResumptionSection);

  // Open attempt timeout in ms
  ReadUIntValue(&resumption_delay_after_ign_, kDefaultResumptionDelayAfterIgn,
                kResumptionSection, kResumptionDelayAfterIgnKey);

  LOG_UPDATED_VALUE(resumption_delay_after_ign_, kResumptionDelayAfterIgnKey,
                    kResumptionSection);

  // Application directory quota
  ReadUIntValue(&app_dir_quota_, kDefaultDirQuota, kMainSection,
                kAppDirectoryQuotaKey);

  if (app_dir_quota_ <= 0) {
    app_dir_quota_ = kDefaultDirQuota;
  }

  LOG_UPDATED_VALUE(app_dir_quota_, kAppDirectoryQuotaKey, kMainSection);

  // TTS delimiter
  // Should be gotten before any TTS prompts, since it should be appended back
  ReadStringValue(&tts_delimiter_, kDefaultTtsDelimiter,
                  kGlobalPropertiesSection, kTTSDelimiterKey);

  LOG_UPDATED_VALUE(tts_delimiter_, kTTSDelimiterKey, kGlobalPropertiesSection);

  // Help prompt
  help_prompt_.clear();
  std::string help_prompt_value;
  if (ReadValue(&help_prompt_value, kGlobalPropertiesSection, kHelpPromptKey)) {
    char* str = NULL;
    str = strtok(const_cast<char*>(help_prompt_value.c_str()), ",");
    while (str != NULL) {
      // Default prompt should have delimiter included for each item
      const std::string prompt_item = std::string(str) + tts_delimiter_;
      help_prompt_.push_back(prompt_item);
      LOG_UPDATED_VALUE(prompt_item, kHelpPromptKey, kGlobalPropertiesSection);
      str = strtok(NULL, ",");
    }
  } else {
    help_prompt_value.clear();
    LOG_UPDATED_VALUE(help_prompt_value, kHelpPromptKey,
                      kGlobalPropertiesSection);
  }

  // Timeout prompt
  time_out_promt_.clear();
  std::string timeout_prompt_value;
  if (ReadValue(&timeout_prompt_value, kGlobalPropertiesSection,
                kTimeoutPromptKey)) {
    char* str = NULL;
    str = strtok(const_cast<char*>(timeout_prompt_value.c_str()), ",");
    while (str != NULL) {
      // Default prompt should have delimiter included for each item
      const std::string prompt_item = std::string(str) + tts_delimiter_;
      time_out_promt_.push_back(prompt_item);
      LOG_UPDATED_VALUE(prompt_item, kTimeoutPromptKey,
                        kGlobalPropertiesSection);
      str = strtok(NULL, ",");
    }
  } else {
    timeout_prompt_value.clear();
    LOG_UPDATED_VALUE(timeout_prompt_value, kTimeoutPromptKey,
                      kGlobalPropertiesSection);
  }

  // Voice recognition help title
  ReadStringValue(&vr_help_title_, "", kGlobalPropertiesSection, kHelpTitleKey);

  LOG_UPDATED_VALUE(vr_help_title_, kHelpTitleKey, kGlobalPropertiesSection);

  // Voice recognition help command
  vr_commands_.clear();
  std::string vr_help_command_value;
  if (ReadValue(&vr_help_command_value, kVrCommandsSection, kHelpCommandKey)) {
    char* str = NULL;
    str = strtok(const_cast<char*>(vr_help_command_value.c_str()), ",");
    while (str != NULL) {
      const std::string vr_item = str;
      vr_commands_.push_back(vr_item);
      LOG_UPDATED_VALUE(vr_item, kHelpCommandKey, kVrCommandsSection);
      str = strtok(NULL, ",");
    }
  } else {
    vr_help_command_value.clear();
    LOG_UPDATED_VALUE(vr_help_command_value, kHelpCommandKey,
                      kVrCommandsSection);
  }

  //TTS GlobalProperties timeout
  ReadUIntValue(&tts_global_properties_timeout_,
                kDefaultTTSGlobalPropertiesTimeout, kGlobalPropertiesSection,
                kTTSGlobalPropertiesTimeoutKey);

  LOG_UPDATED_VALUE(tts_global_properties_timeout_,
                    kTTSGlobalPropertiesTimeoutKey, kGlobalPropertiesSection);

  // Application time scale maximum requests
  ReadUIntValue(&app_time_scale_max_requests_, kDefaultAppTimeScaleMaxRequests,
                kMainSection, kAppTimeScaleMaxRequestsKey);

  LOG_UPDATED_VALUE(app_time_scale_max_requests_, kAppTimeScaleMaxRequestsKey,
                    kMainSection);

  // Application time scale
  ReadUIntValue(&app_requests_time_scale_, kDefaultAppRequestsTimeScale,
                kMainSection, kAppRequestsTimeScaleKey);

  LOG_UPDATED_VALUE(app_requests_time_scale_, kAppRequestsTimeScaleKey,
                    kMainSection);

  // Application HMI level NONE time scale maximum requests
  ReadUIntValue(&app_hmi_level_none_time_scale_max_requests_,
                kDefaultAppHmiLevelNoneTimeScaleMaxRequests, kMainSection,
                kAppHmiLevelNoneTimeScaleMaxRequestsKey);

  LOG_UPDATED_VALUE(app_hmi_level_none_time_scale_max_requests_,
                    kAppHmiLevelNoneTimeScaleMaxRequestsKey, kMainSection);

  // Application HMI level NONE requests time scale
  ReadUIntValue(&app_hmi_level_none_requests_time_scale_,
                kDefaultAppHmiLevelNoneRequestsTimeScale, kMainSection,
                kAppHmiLevelNoneRequestsTimeScaleKey);

  LOG_UPDATED_VALUE(app_hmi_level_none_requests_time_scale_,
                    kAppHmiLevelNoneRequestsTimeScaleKey, kMainSection);

  // Amount of pending requests
  ReadUIntValue(&pending_requests_amount_, kDefaultPendingRequestsAmount,
                kMainSection, kPendingRequestsAmoundKey);

  if (pending_requests_amount_ <= 0) {
    pending_requests_amount_ = kDefaultPendingRequestsAmount;
  }

  LOG_UPDATED_VALUE(pending_requests_amount_, kPendingRequestsAmoundKey,
                    kMainSection);

  // Supported diagnostic modes
  supported_diag_modes_.clear();
  std::string supported_diag_modes_value;
  std::string correct_diag_modes;
  if (ReadStringValue(&supported_diag_modes_value, "", kMainSection,
                      kSupportedDiagModesKey)) {
    char* str = NULL;
    str = strtok(const_cast<char*>(supported_diag_modes_value.c_str()), ",");
    while (str != NULL) {
      uint32_t user_value = strtol(str, NULL, 16);
      if (user_value && errno != ERANGE) {
        correct_diag_modes += str;
        correct_diag_modes += ",";
        supported_diag_modes_.push_back(user_value);
      }
      str = strtok(NULL, ",");
    }
  }

  LOG_UPDATED_VALUE(correct_diag_modes, kSupportedDiagModesKey, kMainSection);

  // System files path
  ReadStringValue(&system_files_path_, kDefaultSystemFilesPath, kMainSection,
                  kSystemFilesPathKey);

  LOG_UPDATED_VALUE(system_files_path_, kSystemFilesPathKey, kMainSection);

  // Heartbeat timeout
  ReadUIntValue(&heart_beat_timeout_, kDefaultHeartBeatTimeout, kMainSection,
                kHeartBeatTimeoutKey);

  LOG_UPDATED_VALUE(heart_beat_timeout_, kHeartBeatTimeoutKey, kMainSection);

  // Use last state value
  ReadBoolValue(&use_last_state_, false, kMainSection, kUseLastStateKey);

  LOG_UPDATED_BOOL_VALUE(use_last_state_, kUseLastStateKey, kMainSection);

  // Transport manager TCP port
  ReadUIntValue(&transport_manager_tcp_adapter_port_,
                kDefautTransportManagerTCPPort, kTransportManagerSection,
                kTCPAdapterPortKey);

  LOG_UPDATED_VALUE(transport_manager_tcp_adapter_port_, kTCPAdapterPortKey,
                    kTransportManagerSection);

  // MME database name
  ReadStringValue(&mme_db_name_, kDefaultMmeDatabaseName,
                  kTransportManagerSection, kMmeDatabaseNameKey);

  LOG_UPDATED_VALUE(mme_db_name_, kMmeDatabaseNameKey, kTransportManagerSection);
  // Event MQ
  ReadStringValue(&event_mq_name_, kDefaultEventMQ, kTransportManagerSection,
                  kEventMQKey);

  LOG_UPDATED_VALUE(event_mq_name_, kEventMQKey, kTransportManagerSection);

  // Ack MQ
  ReadStringValue(&ack_mq_name_, kDefaultAckMQ, kTransportManagerSection,
                  kAckMQKey);

  LOG_UPDATED_VALUE(ack_mq_name_, kAckMQKey, kTransportManagerSection);

  // Transport manager disconnect timeout
  ReadUIntValue(&transport_manager_disconnect_timeout_,
                kDefaultTransportManagerDisconnectTimeout,
                kTransportManagerSection,
                kTransportManagerDisconnectTimeoutKey);

  LOG_UPDATED_VALUE(transport_manager_disconnect_timeout_,
                    kTransportManagerDisconnectTimeoutKey,
                    kTransportManagerSection);

  // Recording file
  ReadStringValue(&recording_file_name_, kDefaultRecordingFileName,
                  kMediaManagerSection, kRecordingFileNameKey);

  LOG_UPDATED_VALUE(recording_file_name_, kRecordingFileNameKey,
                    kMediaManagerSection);

  // Recording file source
  ReadStringValue(&recording_file_source_, kDefaultRecordingFileSourceName,
                  kMediaManagerSection, kRecordingFileSourceKey);

  LOG_UPDATED_VALUE(recording_file_source_, kRecordingFileSourceKey,
                    kMediaManagerSection);

  // Policy preloaded file
  ReadStringValue(&preloaded_pt_file_, kDefaultPreloadedPTFileName,
                  kPolicySection, kPreloadedPTKey);

  preloaded_pt_file_ = app_config_folder_ + '/' + preloaded_pt_file_;

  LOG_UPDATED_VALUE(preloaded_pt_file_, kPreloadedPTKey, kPolicySection);

  // Policy snapshot file
  ReadStringValue(&policy_snapshot_file_name_, kDefaultPoliciesSnapshotFileName,
                  kPolicySection, kPathToSnapshotKey);

  LOG_UPDATED_VALUE(policy_snapshot_file_name_, kPathToSnapshotKey,
                    kPolicySection);

  // Attempts number for opening policy DB
  ReadUIntValue(&attempts_to_open_policy_db_, kDefaultAttemptsToOpenPolicyDB,
                kPolicySection, kAttemptsToOpenPolicyDBKey);

  LOG_UPDATED_VALUE(attempts_to_open_policy_db_, kAttemptsToOpenPolicyDBKey,
                    kPolicySection);

  // Open attempt timeout in ms
  ReadUIntValue(&open_attempt_timeout_ms_, kDefaultOpenAttemptTimeoutMsKey,
                kPolicySection, kOpenAttemptTimeoutMsKey);

  LOG_UPDATED_VALUE(open_attempt_timeout_ms_, kOpenAttemptTimeoutMsKey,
                    kPolicySection);

  // Turn Policy Off?
  ReadBoolValue(&enable_policy_, false, kPolicySection, kEnablePolicy);

  LOG_UPDATED_BOOL_VALUE(enable_policy_, kEnablePolicy, kPolicySection);

  ReadUIntValue(&application_list_update_timeout_,
                kDefaultApplicationListUpdateTimeout,
                kApplicationManagerSection, kApplicationListUpdateTimeoutKey);

  LOG_UPDATED_VALUE(application_list_update_timeout_,
                    kApplicationListUpdateTimeoutKey, kMainSection);

  ReadUintIntPairValue(&read_did_frequency_, kReadDIDFrequency, kMainSection,
                       kReadDIDFrequencykey);

  ReadUintIntPairValue(&get_vehicle_data_frequency_, kGetVehicleDataFrequency,
                       kMainSection, kGetVehicleDataFrequencyKey);

  ReadUIntValue(&max_thread_pool_size_, kDefaultMaxThreadPoolSize,
                kApplicationManagerSection, kDefaultThreadPoolSize);
  if (max_thread_pool_size_ > kDefaultMaxThreadPoolSize) {
    max_thread_pool_size_ = kDefaultMaxThreadPoolSize;
  }
  LOG_UPDATED_VALUE(max_thread_pool_size_, kDefaultMaxThreadPoolSize,
                    kApplicationManagerSection);

  ReadStringValue(&iap_legacy_protocol_mask_, kDefaultLegacyProtocolMask,
                  kIAPSection, kLegacyProtocolMaskKey);

  LOG_UPDATED_VALUE(iap_legacy_protocol_mask_, kLegacyProtocolMaskKey,
                    kIAPSection);

  ReadStringValue(&iap_hub_protocol_mask_, kDefaultHubProtocolMask, kIAPSection,
                  kHubProtocolMaskKey);

  LOG_UPDATED_VALUE(iap_hub_protocol_mask_, kHubProtocolMaskKey, kIAPSection);

  ReadStringValue(&iap_pool_protocol_mask_, kDefaultPoolProtocolMask,
                  kIAPSection, kPoolProtocolMaskKey);

  LOG_UPDATED_VALUE(iap_pool_protocol_mask_, kPoolProtocolMaskKey, kIAPSection);

  ReadStringValue(&iap_system_config_, kDefaultIAPSystemConfig, kIAPSection,
                  kIAPSystemConfigKey);

  LOG_UPDATED_VALUE(iap_system_config_, kIAPSystemConfigKey, kIAPSection);

  ReadStringValue(&iap2_system_config_, kDefaultIAP2SystemConfig, kIAPSection,
                  kIAP2SystemConfigKey);

  LOG_UPDATED_VALUE(iap2_system_config_, kIAP2SystemConfigKey, kIAPSection);

  ReadIntValue(&iap2_hub_connect_attempts_, kDefaultIAP2HubConnectAttempts,
               kIAPSection, kIAP2HubConnectAttemptskey);

  LOG_UPDATED_VALUE(iap2_hub_connect_attempts_, kIAP2HubConnectAttemptskey,
                    kIAPSection);

  ReadIntValue(&iap_hub_connection_wait_timeout_,
               kDefaultIAPHubConnectionWaitTimeout, kIAPSection,
               kIAPHubConnectionWaitTimeoutKey);

  LOG_UPDATED_VALUE(iap_hub_connection_wait_timeout_,
                    kIAPHubConnectionWaitTimeoutKey, kIAPSection);

  ReadIntValue(&iap_arm_event_timeout_,
               kDefaultIAPArmEventTimeout,
               kIAPSection,
               kIAPArmEventTimeoutKey);

  LOG_UPDATED_VALUE(iap_arm_event_timeout_, kDefaultIAPArmEventTimeout, kIAPSection);

  ReadUIntValue(&default_hub_protocol_index_, kDefaultHubProtocolIndex, kIAPSection, kDefaultHubProtocolIndexKey);

  LOG_UPDATED_VALUE(default_hub_protocol_index_, kDefaultHubProtocolIndexKey,
                    kIAPSection);

  ReadUIntValue(&hash_string_size_, kDefaultHashStringSize,
                kApplicationManagerSection, kHashStringSizeKey);

  LOG_UPDATED_VALUE(hash_string_size_, kHashStringSizeKey,
                    kApplicationManagerSection);
}

bool Profile::ReadValue(bool* value, const char* const pSection,
                        const char* const pKey) const {
  DCHECK(value);
  bool ret = false;

  char buf[INI_LINE_LEN + 1];
  *buf = '\0';
  if ((0 != ini_read_value(config_file_name_.c_str(), pSection, pKey, buf))
      && ('\0' != *buf)) {

    const int32_t tmpVal = atoi(buf);
    if ((0 == strcmp("true", buf)) || (0 != tmpVal)) {
      *value = true;
    } else {
      *value = false;
    }
    ret = true;
  }
  return ret;
}

bool Profile::ReadValue(std::string* value, const char* const pSection,
                        const char* const pKey) const {
  DCHECK(value);
  bool ret = false;

  char buf[INI_LINE_LEN + 1];
  *buf = '\0';
  if ((0 != ini_read_value(config_file_name_.c_str(), pSection, pKey, buf))
      && ('\0' != *buf)) {
    *value = buf;
    ret = true;
  }
  return ret;
}

bool Profile::ReadStringValue(std::string* value, const char* default_value,
                              const char* const pSection,
                              const char* const pKey) const {
  DCHECK(value);
  if (!ReadValue(value, pSection, pKey)) {
    *value = default_value;
    return false;
  }
  return true;
}

bool Profile::ReadIntValue(int32_t* value, const int32_t default_value,
                           const char* const pSection,
                           const char* const pKey) const {
  DCHECK(value);
  std::string string_value;
  if (!ReadValue(&string_value, pSection, pKey)) {
    *value = default_value;
    return false;
  }
  *value = atoi(string_value.c_str());
  return true;
}

bool Profile::ReadUintIntPairValue(
    std::pair<uint32_t, int32_t>* value,
    const std::pair<uint32_t, uint32_t>& default_value,
    const char * const pSection, const char * const pKey) const {
  std::string string_value;
  if (!ReadValue(&string_value, pSection, pKey)) {
    *value = default_value;
    return false;
  }
  std::string first_str = string_value.substr(0, string_value.find(","));
  std::string second_str = string_value.substr(
      string_value.find(",") + 1, string_value.size() - first_str.size());
  (*value).first = strtoul(first_str.c_str(), NULL, 10);
  (*value).second = strtoul(second_str.c_str(), NULL, 10);
  return true;
}

bool Profile::ReadBoolValue(bool* value, const bool default_value,
                            const char* const pSection,
                            const char* const pKey) const {
  DCHECK(value);
  bool read_value;
  const bool result = ReadValue(&read_value, pSection, pKey);
  *value = result ? read_value : default_value;
  return result;
}
namespace {
int32_t hex_to_int(const std::string& value) {
  return static_cast<int32_t>(strtol(value.c_str(), NULL, 16));
}
}

std::list<int> Profile::ReadIntContainer(const char * const pSection,
                                         const char * const pKey,
                                         bool *out_result) const {
  const std::list<std::string> string_list = ReadStringContainer(pSection, pKey,
                                                                 out_result);
  std::list<int> value_list;
  value_list.resize(string_list.size());
  std::transform(string_list.begin(), string_list.end(), value_list.begin(),
                 hex_to_int);
  return value_list;
}

std::list<std::string> Profile::ReadStringContainer(const char * const pSection,
                                                    const char * const pKey,
                                                    bool *out_result) const {
  std::string string;
  const bool result = ReadValue(&string, pSection, pKey);
  if (out_result)
    *out_result = result;
  std::list < std::string > value_container;
  if (result) {
    std::istringstream iss(string);
    std::string temp_str;
    while (iss) {
      if (!getline(iss, temp_str, ','))
        break;
      value_container.push_back(temp_str);
    }
  }
  return value_container;
}

bool Profile::ReadUIntValue(uint16_t* value, uint16_t default_value,
                            const char* const pSection,
                            const char* const pKey) const {
  std::string string_value;
  if (!ReadValue(&string_value, pSection, pKey)) {
    *value = default_value;
    return false;
  } else {
    uint64_t user_value = strtoul(string_value.c_str(), NULL, 10);
    if (!user_value || errno == ERANGE
        || (user_value > std::numeric_limits < uint16_t > ::max())) {
      *value = default_value;
      return false;
    }

    *value = user_value;
    return true;
  }
}

bool Profile::ReadUIntValue(uint32_t* value, uint32_t default_value,
                            const char* const pSection,
                            const char* const pKey) const {
  std::string string_value;
  if (!ReadValue(&string_value, pSection, pKey)) {
    *value = default_value;
    return false;
  } else {
    uint64_t user_value = strtoul(string_value.c_str(), NULL, 10);
    if (!user_value || errno == ERANGE
        || (user_value > std::numeric_limits < uint32_t > ::max())) {
      *value = default_value;
      return false;
    }
    *value = user_value;
    return true;
  }
}

bool Profile::ReadUIntValue(uint64_t* value, uint64_t default_value,
                            const char* const pSection,
                            const char* const pKey) const {
  std::string string_value;
  if (!ReadValue(&string_value, pSection, pKey)) {
    *value = default_value;
    return false;
  } else {
    uint64_t user_value = strtoull(string_value.c_str(), NULL, 10);
    if (!user_value || errno == ERANGE) {
      *value = default_value;
      return false;
    }

    *value = user_value;
    return true;
  }
}

void Profile::LogContainer(const std::vector<std::string>& container,
                           std::string* log) {
  if (container.empty()) {
    return;
  }
  if (NULL == log) {
    return;
  }
  std::vector<std::string>::const_iterator it = container.begin();
  std::vector<std::string>::const_iterator it_end = container.end();
  for (; it != it_end - 1; ++it) {
    log->append(*it);
    log->append(" ; ");
  }

  log->append(container.back());
}
}  //  namespace profile
