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
#ifndef SRC_COMPONENTS_TRANSPORT_MANAGER_INCLUDE_TRANSPORT_MANAGER_AOA_PPS_LISTENER_H_
#define SRC_COMPONENTS_TRANSPORT_MANAGER_INCLUDE_TRANSPORT_MANAGER_AOA_PPS_LISTENER_H_

#include <mqueue.h>
#include <string>
#include <map>
#include <utility>

#include "utils/lock.h"
#include "utils/threads/thread.h"
#include "utils/threads/pulse_thread_delegate.h"
#include "utils/atomic_object.h"

#include "transport_manager/transport_adapter/client_connection_listener.h"
#include "transport_manager/aoa/aoa_wrapper.h"


namespace transport_manager {
namespace transport_adapter {

typedef std::map<AOAWrapper::AOAUsbInfo, DeviceUID> DeviceCollection;
typedef DeviceCollection::value_type DeviceCollectionPair;

class AOATransportAdapter;

class PPSListener : public ClientConnectionListener {
  class MQHandler {
   public:
    /**
     * @breif Creates MQHandler instance.
     * Will try to open specified mqueue with certain flags
     *
     * @param name - the name of mqueue to connect.
     * @param flags - the mode with which certain mqueue will be opened.
     */
    MQHandler(const std::string& name, int flags);

    /**
     * @breif Allows to send message into the certain mqueue.
     *
     * @param message - the message for writing into the mqueue.
     */
    void Write(const std::vector<char>& message) const;

    /**
     * @breif Allows to read message from the mqueue.
     *
     * @param data - the out parameter. Will be filled by the method.
     * In case the method is unable receive the data it will be resized to 0.
     */
    void Read(std::vector<char>& data) const;

    /**
     * @breif Close the certain mqueue.
     */
    ~MQHandler();

   private:

    /**
     * @breif Used by constructor in order to initialize and open certain mqueue.
     *
     * @param name - mqueue name to opent.
     * @param flags - the open mode.
     */
    void init_mq(const std::string& name, int flags);

    /**
     * @breif Allows to close certain mqueue.
     */
    void deinit_mq();

    mqd_t handle_;
  };
 public:
  explicit PPSListener(AOATransportAdapter* controller);
  ~PPSListener();

  TransportAdapter::Error Init() OVERRIDE;
  void Terminate() OVERRIDE;
  bool IsInitialised() const OVERRIDE;
  TransportAdapter::Error StartListening() OVERRIDE;
  TransportAdapter::Error StopListening() OVERRIDE;

  void ReceiveMQMessage(std::vector<char>& message);
  void SendMQMessage(const std::vector<char>& message);

 private:
  bool OpenPps();
  void ClosePps();
  bool ArmEvent(const struct sigevent* event);
  void ProcessPpsMessage(uint8_t* message, const size_t size);
  std::string ParsePpsData(char* ppsdata, const char** vals);
  void SwitchMode(const AOAWrapper::AOAUsbInfo& usb_info);
  bool IsAOADevice(const char** attrs);
  void FillUsbInfo(const char** attrs,
                   AOAWrapper::AOAUsbInfo* info);
  bool IsAOAMode(const AOAWrapper::AOAUsbInfo& aoa_usb_info);
  void AddDevice(const AOAWrapper::AOAUsbInfo& aoa_usb_info);
  void DisconnectDevice(const DeviceCollectionPair device);
  void ProcessAOADevice(const DeviceCollectionPair device);
  bool init_aoa();
  void release_aoa() const;

  bool initialised_;
  AOATransportAdapter* controller_;
  int fd_;
  threads::Thread* pps_thread_;
  threads::Thread* mq_thread_;
  mutable sync_primitives::atomic_bool is_aoa_available_;
  threads::Thread* thread_;

  MQHandler mq_from_applink_handle_;
  MQHandler mq_to_applink_handle_;

  // Own collection for handling after release
  DeviceCollection devices_;
  mutable sync_primitives::Lock devices_lock_;

  // releasing and taking AOA signal handler
  class PpsMQListener: public threads::ThreadDelegate {
   public:
    explicit PpsMQListener(PPSListener* parent);
    void threadMain() OVERRIDE;
    void exitThreadMain() OVERRIDE;
   private:
    void take_aoa();
    void release_aoa();

    PPSListener* parent_;
    sync_primitives::atomic_bool run_;
  };

  // QNX PPS system signals handler
  class PpsThreadDelegate : public threads::PulseThreadDelegate {
   public:
    explicit PpsThreadDelegate(PPSListener* parent);

   protected:
    bool ArmEvent(struct sigevent* event) OVERRIDE;
    void OnPulse() OVERRIDE {}

   private:
    PPSListener* parent_;
  };
};

}  // namespace transport_adapter
}  // namespace transport_manager
#endif  // SRC_COMPONENTS_TRANSPORT_MANAGER_INCLUDE_TRANSPORT_MANAGER_AOA_PPS_LISTENER_H_
