//
// Copyright (c) 2013, Ford Motor Company
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// Redistributions of source code must retain the above copyright notice, this
// list of conditions and the following disclaimer.
//
// Redistributions in binary form must reproduce the above copyright notice,
// this list of conditions and the following
// disclaimer in the documentation and/or other materials provided with the
// distribution.
//
// Neither the name of the Ford Motor Company nor the names of its contributors
// may be used to endorse or promote products derived from this software
// without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//

#if defined(OS_POSIX) && defined(OS_LINUX)
#include <pthread.h>  // TODO(DK): Need to remove
#endif

#include <string>
#include "application_manager/audio_pass_thru_thread_impl.h"
#include "application_manager/application_manager_impl.h"
#include "application_manager/mobile_command_factory.h"
#include "application_manager/application_impl.h"
#include "smart_objects/smart_object.h"
#include "interfaces/MOBILE_API.h"
#include "utils/file_system.h"
#include "utils/timer.h"

namespace application_manager {

const int AudioPassThruThreadImpl::kAudioPassThruTimeout = 1;
log4cxx::LoggerPtr AudioPassThruThreadImpl::logger_ = log4cxx::LoggerPtr(
    log4cxx::Logger::getLogger("AudioPassThruThread"));

AudioPassThruThreadImpl::AudioPassThruThreadImpl(
    const std::string fileName, unsigned int session_key,
    unsigned int correlation_id, unsigned int max_duration,
    const SamplingRate& sampling_rate,
    const AudioCaptureQuality& bits_per_sample, const AudioType& audio_type)
    : session_key_(session_key),
      correlation_id_(correlation_id),
      max_duration_(max_duration),
      sampling_rate_(sampling_rate),
      bits_per_sample_(bits_per_sample),
      audio_type_(audio_type),
      timer_(NULL),
      fileName_(fileName) {
  LOG4CXX_TRACE_ENTER(logger_);
  stopFlagMutex_.init();
}

AudioPassThruThreadImpl::~AudioPassThruThreadImpl() {
  if (timer_) {
    delete timer_;
  }
}

void AudioPassThruThreadImpl::Init() {
  LOG4CXX_TRACE_ENTER(logger_);
  synchronisation_.init();
  timer_ = new sync_primitives::Timer(&synchronisation_);
  if (!timer_) {
    LOG4CXX_ERROR_EXT(logger_, "Init NULL pointer");
  }
}

void AudioPassThruThreadImpl::FactoryCreateCommand(
    smart_objects::SmartObject* cmd) {
  CommandSharedPtr command = MobileCommandFactory::CreateCommand(&(*cmd));
  command->Init();
  command->Run();
  command->CleanUp();
}

bool AudioPassThruThreadImpl::SendEndAudioPassThru() {
  LOG4CXX_INFO(logger_, "sendEndAudioPassThruToHMI");

  smart_objects::SmartObject* end_audio = new smart_objects::SmartObject();
  if (NULL == end_audio) {
    smart_objects::SmartObject* error_response =
        new smart_objects::SmartObject();
    if (NULL != error_response) {
      (*error_response)[strings::params][strings::message_type] =
          MessageType::kResponse;
      (*error_response)[strings::params][strings::correlation_id] =
          static_cast<int>(correlation_id_);

      (*error_response)[strings::params][strings::connection_key] =
          static_cast<int>(session_key_);
      (*error_response)[strings::params][strings::function_id] =
          mobile_apis::FunctionID::PerformAudioPassThruID;

      (*error_response)[strings::msg_params][strings::success] = false;
      (*error_response)[strings::msg_params][strings::result_code] =
          mobile_apis::Result::OUT_OF_MEMORY;
      FactoryCreateCommand(error_response);
    }
    return false;
  }

  Application* app = ApplicationManagerImpl::instance()->application(
      session_key_);

  if (!app) {
    LOG4CXX_ERROR_EXT(logger_, "APPLICATION_NOT_REGISTERED");
    return false;
  }

  (*end_audio)[strings::params][strings::message_type] = MessageType::kResponse;
  (*end_audio)[strings::params][strings::correlation_id] =
      static_cast<int>(correlation_id_);

  (*end_audio)[strings::params][strings::connection_key] =
      static_cast<int>(session_key_);
  (*end_audio)[strings::params][strings::function_id] =
      mobile_apis::FunctionID::EndAudioPassThruID;

  (*end_audio)[strings::msg_params][strings::success] = true;
  (*end_audio)[strings::msg_params][strings::result_code] =
      mobile_apis::Result::SUCCESS;

  // app_id
  (*end_audio)[strings::msg_params][strings::app_id] = app->app_id();
  FactoryCreateCommand(end_audio);
  return true;
}

void AudioPassThruThreadImpl::threadMain() {
  LOG4CXX_TRACE_ENTER(logger_);

  offset_ = 0;

  stopFlagMutex_.lock();
  shouldBeStoped_ = false;
  stopFlagMutex_.unlock();

  while (true) {

    sendAudioChunkToMobile();

    stopFlagMutex_.lock();
    if (shouldBeStoped_) {
       break;
    }
    stopFlagMutex_.unlock();
  }
}

void AudioPassThruThreadImpl::sendAudioChunkToMobile() {
  LOG4CXX_TRACE_ENTER(logger_);

  stopFlagMutex_.lock();
  if (shouldBeStoped_) {
    return;
  }
  stopFlagMutex_.unlock();

  std::vector<unsigned char> binaryData;
  std::vector<unsigned char>::iterator from;
  std::vector<unsigned char>::iterator to;

  timer_->StartWait(kAudioPassThruTimeout);

  if (!file_system::ReadBinaryFile(fileName_, binaryData)) {
#if defined(OS_POSIX) && defined(OS_LINUX)
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
#endif

    LOG4CXX_ERROR_EXT(logger_, "Unable to read file." << fileName_);
    if (false == SendEndAudioPassThru()) {
      LOG4CXX_ERROR_EXT(logger_, "Unable to send EndAudioPassThru");
    }

    ApplicationManagerImpl::instance()->set_audio_pass_thru_flag(false);
  }

  if (binaryData.empty()) {
    LOG4CXX_ERROR_EXT(logger_, "Binary data is empty.");
    return;
  }

  LOG4CXX_INFO_EXT(logger_, "offset = " << offset_);

  from = binaryData.begin() + offset_;
  to = binaryData.end();

  if (from < binaryData.end() /*from != binaryData.end()*/) {

    LOG4CXX_INFO_EXT(logger_, "from != binaryData.end()");

    offset_ = offset_ + to - from;

    LOG4CXX_INFO_EXT(logger_, "Create smart object");

    smart_objects::SmartObject* on_audio_pass = NULL;
    on_audio_pass = new smart_objects::SmartObject();
    if (NULL == on_audio_pass) {
      LOG4CXX_ERROR_EXT(logger_, "OnAudioPassThru NULL pointer");
      if (false == SendEndAudioPassThru()) {
        LOG4CXX_ERROR_EXT(logger_, "Unable to send EndAudioPassThru");
      }

      ApplicationManagerImpl::instance()->set_audio_pass_thru_flag(false);

      return;
    }

    LOG4CXX_INFO_EXT(logger_, "Fill smart object");

    (*on_audio_pass)[strings::params][strings::message_type] =
        MessageType::kNotification;
    (*on_audio_pass)[strings::params][strings::correlation_id] =
        static_cast<int>(correlation_id_);

    (*on_audio_pass)[strings::params][strings::connection_key] =
        static_cast<int>(session_key_);
    (*on_audio_pass)[strings::params][strings::function_id] =
        mobile_apis::FunctionID::OnAudioPassThruID;

    LOG4CXX_INFO_EXT(logger_, "Fill binary data");
    // binary data
    (*on_audio_pass)[strings::params][strings::binary_data] =
        smart_objects::SmartObject(std::vector<unsigned char>(from, to));

    LOG4CXX_INFO_EXT(logger_, "After fill binary data");

    binaryData.clear();

    LOG4CXX_INFO_EXT(logger_, "Send data");

    FactoryCreateCommand(on_audio_pass);
  }

}

void AudioPassThruThreadImpl::exitThreadMain() {
  LOG4CXX_INFO(logger_, "AudioPassThruThreadImpl::exitThreadMain");

  stopFlagMutex_.lock();
  shouldBeStoped_ = true;
  stopFlagMutex_.unlock();
}

unsigned int AudioPassThruThreadImpl::session_key() const {
  return session_key_;
}

unsigned int AudioPassThruThreadImpl::correlation_id() const {
  return correlation_id_;
}

unsigned int AudioPassThruThreadImpl::max_duration() const {
  return max_duration_;
}

const SamplingRate& AudioPassThruThreadImpl::sampling_rate() const {
  return sampling_rate_;
}

const AudioCaptureQuality& AudioPassThruThreadImpl::bits_per_sample() const {
  return bits_per_sample_;
}

const AudioType& AudioPassThruThreadImpl::audio_type() const {
  return audio_type_;
}
}  // namespace application_manager
