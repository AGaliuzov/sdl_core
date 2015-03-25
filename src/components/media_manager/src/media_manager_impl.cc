/*
 * Copyright (c) 2014, Ford Motor Company
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
#include "media_manager/media_manager_impl.h"
#include "media_manager/audio/from_mic_recorder_listener.h"
#include "media_manager/streamer_listener.h"
#include "application_manager/message_helper.h"
#include "application_manager/application.h"
#include "application_manager/application_manager_impl.h"
#include "application_manager/application_impl.h"
#include "utils/file_system.h"
#include "utils/logger.h"
#include "utils/helpers.h"
#if defined(EXTENDED_MEDIA_MODE)
#include "media_manager/audio/a2dp_source_player_adapter.h"
#include "media_manager/audio/from_mic_recorder_adapter.h"
#endif
#include "media_manager/video/socket_video_streamer_adapter.h"
#include "media_manager/audio/socket_audio_streamer_adapter.h"
#include "media_manager/video/pipe_video_streamer_adapter.h"
#include "media_manager/audio/pipe_audio_streamer_adapter.h"
#include "media_manager/video/video_stream_to_file_adapter.h"


namespace media_manager {

using profile::Profile;
using protocol_handler::ServiceType;
using timer::TimerThread;

CREATE_LOGGERPTR_GLOBAL(logger_, "MediaManagerImpl")

MediaManagerImpl::MediaManagerImpl()
  : protocol_handler_(NULL)
  , a2dp_player_(NULL)
  , from_mic_recorder_(NULL)
  , video_stream_active_(false)
  , audio_stream_active_(false)
  , audio_streaming_timer_("Audio streaming timer", this,
                           &MediaManagerImpl::OnAudioStreamingTimeout)
  , video_streaming_timer_("Video streaming timer", this,
                           &MediaManagerImpl::OnVideoStreamingTimeout)
  , audio_streaming_suspended_(true)
  , video_streaming_suspended_(true)
  , streaming_app_id_(0) {
  Init();
}

MediaManagerImpl::~MediaManagerImpl() {

  if (a2dp_player_) {
    delete a2dp_player_;
    a2dp_player_ = NULL;
  }

  if (from_mic_recorder_) {
    delete from_mic_recorder_;
    from_mic_recorder_ = NULL;
  }

}

void MediaManagerImpl::SetProtocolHandler(
  protocol_handler::ProtocolHandler* protocol_handler) {
  protocol_handler_ = protocol_handler;
}

void MediaManagerImpl::Init() {
  LOG4CXX_INFO(logger_, "MediaManagerImpl::Init()");

#if defined(EXTENDED_MEDIA_MODE)
  LOG4CXX_INFO(logger_, "Called Init with default configuration.");
  a2dp_player_ = new A2DPSourcePlayerAdapter();
  from_mic_recorder_ = new FromMicRecorderAdapter();
#endif

  if ("socket" == profile::Profile::instance()->video_server_type()) {
    streamer_[ServiceType::kMobileNav] = new SocketVideoStreamerAdapter();
  } else if ("pipe" == profile::Profile::instance()->video_server_type()) {
    streamer_[ServiceType::kMobileNav]= new PipeVideoStreamerAdapter();
  } else if ("file" == profile::Profile::instance()->video_server_type()) {
    streamer_[ServiceType::kMobileNav] = new VideoStreamToFileAdapter(
        profile::Profile::instance()->video_stream_file());
  }

  if ("socket" == profile::Profile::instance()->audio_server_type()) {
    streamer_[ServiceType::kAudio] = new SocketAudioStreamerAdapter();
  } else if ("pipe" == profile::Profile::instance()->audio_server_type()) {
    streamer_[ServiceType::kAudio] = new PipeAudioStreamerAdapter();
  } else if ("file" == profile::Profile::instance()->audio_server_type()) {
    streamer_[ServiceType::kAudio] = new VideoStreamToFileAdapter(
        profile::Profile::instance()->audio_stream_file());
  }

  // Currently timer does not support ms, so we have to convert value to sec
  audio_data_stopped_timeout_ =
      profile::Profile::instance()->audio_data_stopped_timeout() / 1000;

  video_data_stopped_timeout_ =
      profile::Profile::instance()->video_data_stopped_timeout() / 1000;

  video_streamer_listener_ = new StreamerListener();
  audio_streamer_listener_ = new StreamerListener();

  if (streamer_[ServiceType::kMobileNav]) {
    streamer_[ServiceType::kMobileNav]->AddListener(video_streamer_listener_);
  }

  if (streamer_[ServiceType::kAudio]) {
    streamer_[ServiceType::kAudio]->AddListener(audio_streamer_listener_);
  }
}

void MediaManagerImpl::OnAudioStreamingTimeout() {
  using namespace application_manager;
  using namespace protocol_handler;
  LOG4CXX_DEBUG(logger_, "Data is not available for service type "
                << ServiceType::kAudio);
  MessageHelper::SendOnDataStreaming(ServiceType::kAudio, false);
  ApplicationManagerImpl::instance()->StreamingEnded(streaming_app_id_);

  sync_primitives::AutoLock lock(audio_streaming_suspended_lock_);
  audio_streaming_suspended_ = true;
}

void media_manager::MediaManagerImpl::OnVideoStreamingTimeout() {
  using namespace application_manager;
  using namespace protocol_handler;
  LOG4CXX_DEBUG(logger_, "Data is not available for service type "
                << ServiceType::kMobileNav);
  MessageHelper::SendOnDataStreaming(ServiceType::kMobileNav, false);
  ApplicationManagerImpl::instance()->StreamingEnded(streaming_app_id_);

  sync_primitives::AutoLock lock(video_streaming_suspended_lock_);
  video_streaming_suspended_ = true;
}

void MediaManagerImpl::PlayA2DPSource(int32_t application_key) {
  LOG4CXX_AUTO_TRACE(logger_);
  if (a2dp_player_) {
    a2dp_player_->StartActivity(application_key);
  }
}

void MediaManagerImpl::StopA2DPSource(int32_t application_key) {
  LOG4CXX_AUTO_TRACE(logger_);
  if (a2dp_player_) {
    a2dp_player_->StopActivity(application_key);
  }
}

void MediaManagerImpl::StartMicrophoneRecording(
  int32_t application_key,
  const std::string& output_file,
  int32_t duration) {
  LOG4CXX_INFO(logger_, "MediaManagerImpl::StartMicrophoneRecording to "
               << output_file);
  application_manager::ApplicationSharedPtr app =
    application_manager::ApplicationManagerImpl::instance()->
      application(application_key);
  std::string file_path =
#ifdef CUSTOMER_PASA
  profile::Profile::instance()->audio_mq_path();
#else
  profile::Profile::instance()->app_storage_folder();
  file_path += "/";
  file_path += output_file;
#endif // CUSTOMER_PASA
  from_mic_listener_ = new FromMicRecorderListener(file_path);
#if defined(EXTENDED_MEDIA_MODE)
  if (from_mic_recorder_) {
    from_mic_recorder_->AddListener(from_mic_listener_);
    (static_cast<FromMicRecorderAdapter*>(from_mic_recorder_))
    ->set_output_file(file_path);
    (static_cast<FromMicRecorderAdapter*>(from_mic_recorder_))
    ->set_duration(duration);
    from_mic_recorder_->StartActivity(application_key);
  }
#else
  if (file_system::FileExists(file_path)) {
    LOG4CXX_INFO(logger_, "File " << output_file << " exists, removing");
    if (file_system::DeleteFile(file_path)) {
      LOG4CXX_INFO(logger_, "File " << output_file << " removed");
    }
    else {
      LOG4CXX_WARN(logger_, "Could not remove file " << output_file);
    }
  }
#ifndef CUSTOMER_PASA
  const std::string record_file_source =
      profile::Profile::instance()->app_resourse_folder() + "/" +
      profile::Profile::instance()->recording_file_source();
  std::vector<uint8_t> buf;
  if (file_system::ReadBinaryFile(record_file_source, buf)) {
    if (file_system::Write(file_path, buf)) {
      LOG4CXX_INFO(logger_,
        "File " << record_file_source << " copied to " << output_file);
    }
    else {
      LOG4CXX_WARN(logger_, "Could not write to file " << output_file);
    }
  }
  else {
    LOG4CXX_WARN(logger_, "Could not read file " << record_file_source);
  }
#endif
#endif
  from_mic_listener_->OnActivityStarted(application_key);
}

void MediaManagerImpl::StopMicrophoneRecording(int32_t application_key) {
  LOG4CXX_AUTO_TRACE(logger_);
#if defined(EXTENDED_MEDIA_MODE)
  if (from_mic_recorder_) {
    from_mic_recorder_->StopActivity(application_key);
  }
#endif
  if (from_mic_listener_) {
    from_mic_listener_->OnActivityEnded(application_key);
  }
#if defined(EXTENDED_MEDIA_MODE)
  if (from_mic_recorder_) {
    from_mic_recorder_->RemoveListener(from_mic_listener_);
  }
#endif
}

void MediaManagerImpl::StartVideoStreaming(int32_t application_key) {
  LOG4CXX_AUTO_TRACE(logger_);

  if (streamer_[ServiceType::kMobileNav]) {
    if (!video_stream_active_) {
      video_stream_active_ = true;
      streamer_[ServiceType::kMobileNav]->StartActivity(application_key);
      application_manager::MessageHelper::SendNaviStartStream(application_key);
    }
  }
}

void MediaManagerImpl::StopVideoStreaming(int32_t application_key) {
  LOG4CXX_AUTO_TRACE(logger_);
  if (streamer_[ServiceType::kMobileNav]) {
    video_stream_active_ = false;
    application_manager::MessageHelper::SendNaviStopStream(application_key);
    streamer_[ServiceType::kMobileNav]->StopActivity(application_key);
  }
}

void MediaManagerImpl::StartAudioStreaming(int32_t application_key) {
  LOG4CXX_AUTO_TRACE(logger_);

  if (streamer_[ServiceType::kAudio]) {
    if (!audio_stream_active_) {
      audio_stream_active_ = true;
      streamer_[ServiceType::kAudio]->StartActivity(application_key);
      application_manager::MessageHelper::SendAudioStartStream(application_key);
    }
  }
}

void MediaManagerImpl::StopAudioStreaming(int32_t application_key) {
  LOG4CXX_AUTO_TRACE(logger_);
  if (streamer_[ServiceType::kAudio]) {
    audio_stream_active_ = false;
    application_manager::MessageHelper::SendAudioStopStream(application_key);
    streamer_[ServiceType::kAudio]->StopActivity(application_key);
  }
}

void MediaManagerImpl::OnMessageReceived(
    const ::protocol_handler::RawMessagePtr message) {
  LOG4CXX_AUTO_TRACE(logger_);
  using namespace application_manager;
  using namespace helpers;
  streaming_app_id_ = message->connection_key();
  const ServiceType service_type = message->service_type();

  if (Compare<ServiceType, NEQ, ALL>(
        service_type, ServiceType::kMobileNav, ServiceType::kAudio)) {
    LOG4CXX_DEBUG(logger_, "Unsupported service type in MediaManager");
    return;
  }

  if (!CanStream(service_type)) {
    ApplicationManagerImpl::instance()->ForbidStreaming(streaming_app_id_);
    LOG4CXX_ERROR(logger_, "The application trying to stream when it should not.");
    return;
  }

  WakeUpStreaming(service_type);

  streamer_[service_type]->SendData(streaming_app_id_, message);

  if (service_type == ServiceType::kMobileNav) {
    video_streaming_timer_.start(video_data_stopped_timeout_);
  } else {
    audio_streaming_timer_.start(audio_data_stopped_timeout_);
  }
}

void MediaManagerImpl::OnMobileMessageSent(
  const ::protocol_handler::RawMessagePtr message) {
}

void MediaManagerImpl::FramesProcessed(int32_t application_key,
                                       int32_t frame_number) {
  if (protocol_handler_) {
    protocol_handler_->SendFramesNumber(application_key,
                                        frame_number);
  }
}

void MediaManagerImpl::WakeUpStreaming(ServiceType service_type) {
  LOG4CXX_AUTO_TRACE(logger_);
  using namespace application_manager;
  bool stream_woke_up = false;
  if (service_type == ServiceType::kMobileNav) {
    sync_primitives::AutoLock lock(video_streaming_suspended_lock_);
    if (video_streaming_suspended_) {
      video_streaming_suspended_ = false;
      stream_woke_up = true;
    }
  } else {
    sync_primitives::AutoLock lock(audio_streaming_suspended_lock_);
    if (audio_streaming_suspended_) {
      audio_streaming_suspended_ = false;
      stream_woke_up = true;
    }
  }
  if (stream_woke_up) {
    LOG4CXX_DEBUG(logger_, "Data is available for service type " << service_type);
    MessageHelper::SendOnDataStreaming(service_type, true);
  }
}

bool MediaManagerImpl::CanStream(ServiceType service_type) {
  LOG4CXX_AUTO_TRACE(logger_);
  using namespace application_manager;

  if (!streamer_[service_type]) {
    LOG4CXX_DEBUG(logger_, "There is no appropriate streamer");
    return false;
  }

  const bool is_navi = (service_type == ServiceType::kMobileNav);
  const bool is_allowed =
      is_navi ? ApplicationManagerImpl::instance()-> IsVideoStreamingAllowed(
                  streaming_app_id_) :
                ApplicationManagerImpl::instance()-> IsAudioStreamingAllowed(
                  streaming_app_id_);
  return is_allowed;
}

}  //  namespace media_manager
