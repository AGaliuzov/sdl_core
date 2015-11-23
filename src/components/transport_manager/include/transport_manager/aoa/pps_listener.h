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
#ifndef SRC_COMPONENTS_TRANSPORT_MANAGER_INCLUDE_TRANSPORT_MANAGER_PPS_LISTENER_H_
#define SRC_COMPONENTS_TRANSPORT_MANAGER_INCLUDE_TRANSPORT_MANAGER_PPS_LISTENER_H_

#include <string>
#include <map>

#include "utils/lock.h"
#include "utils/threads/thread.h"
#include "utils/threads/pulse_thread_delegate.h"
#include "utils/atomic_object.h"

#include "transport_manager/transport_adapter/client_connection_listener.h"
#include "transport_manager/aoa/aoa_wrapper.h"

#include <mqueue.h>

namespace transport_manager {
namespace transport_adapter {

class AOATransportAdapter;

struct UsbInfoComparator {
  bool operator()(const AOAWrapper::AOAUsbInfo& first,
                  const AOAWrapper::AOAUsbInfo& second ) {
    return first.object_name.compare(second.object_name) < 0;
  }
};

class PPSListener : public ClientConnectionListener {
 public:
  explicit PPSListener(AOATransportAdapter* controller);
  ~PPSListener();

  protected:
  virtual TransportAdapter::Error Init();
  virtual void Terminate();
  virtual TransportAdapter::Error StartListening();
  virtual TransportAdapter::Error StopListening();
  virtual bool IsInitialised() const;

 private:
  static const std::string kUSBStackPath;
  static const std::string kPpsPathRoot;
  static const std::string kPpsPathAll;
  static const std::string kPpsPathCtrl;
  bool initialised_;
  AOATransportAdapter* controller_;
  int fd_;
  threads::Thread* pps_thread_;
  threads::Thread* mq_thread_;
  mutable sync_primitives::atomic_bool is_aoa_available_;
  threads::Thread* thread_;

  typedef std::map<AOAWrapper::AOAUsbInfo,
                  DeviceUID, UsbInfoComparator> DeviceCollection;
  DeviceCollection devices;

  bool OpenPps();
  void ClosePps();
  bool ArmEvent(struct sigevent* event);
  void Process(uint8_t* buf, size_t size);
  std::string ParsePpsData(char* ppsdata, const char** vals);
  void SwitchMode(const AOAWrapper::AOAUsbInfo& usb_info);
  bool IsAOADevice(const char** attrs);
  void FillUsbInfo(const char** attrs,
                   AOAWrapper::AOAUsbInfo* info);
  bool IsAOAMode(const AOAWrapper::AOAUsbInfo& aoa_usb_info);
  void AddDevice(const AOAWrapper::AOAUsbInfo& aoa_usb_info);
  void DisconnectDevice(const std::pair<const AOAWrapper::AOAUsbInfo, DeviceUID> device);
  void ProcessAOADevice(const std::pair<const AOAWrapper::AOAUsbInfo, DeviceUID> device);
  bool init_aoa();
  void release_aoa() const;

  void release_thread(threads::Thread** thread);

  class PpsMQListener: public threads::ThreadDelegate {
   public:
      explicit PpsMQListener(PPSListener* parent);
      virtual void threadMain();
      virtual void exitThreadMain();
   private:
      void take_aoa();
      void release_aoa();
      void init_mq(const std::string& name, int flags, int& descriptor);
      void deinit_mq(mqd_t descriptor);

      PPSListener* parent_;
      mqd_t mq_from_applink_handle_;
      mqd_t mq_to_applink_handle_;

      sync_primitives::atomic_bool run_;
  };

  class PpsThreadDelegate : public threads::PulseThreadDelegate {
   public:
    explicit PpsThreadDelegate(PPSListener* parent);

   protected:
    virtual bool ArmEvent(struct sigevent* event);
    virtual void OnPulse();

   private:
    PPSListener* parent_;
  };
};

}  // namespace transport_adapter
}  // namespace transport_manager



#endif  // SRC_COMPONENTS_TRANSPORT_MANAGER_INCLUDE_TRANSPORT_MANAGER_PPS_LISTENER_H_
