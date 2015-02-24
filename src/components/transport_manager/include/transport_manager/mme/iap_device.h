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

#ifndef SRC_COMPONENTS_TRANSPORT_MANAGER_INCLUDE_TRANSPORT_MANAGER_IAP_DEVICE_H_
#define SRC_COMPONENTS_TRANSPORT_MANAGER_INCLUDE_TRANSPORT_MANAGER_IAP_DEVICE_H_

#include <map>
#include <ipod/ipod.h>

#include "utils/threads/thread.h"
#include "utils/threads/pulse_thread_delegate.h"
#include "utils/lock.h"
#include "utils/timer_thread.h"

#include "transport_manager/mme/mme_device.h"
#include "transport_manager/mme/protocol_connection_timer.h"
#include "transport_manager/transport_adapter/transport_adapter_controller.h"

namespace transport_manager {
namespace transport_adapter {

class IAPConnection;

/**
 * @brief Class representing device connected over iAP protocol
 */
class IAPDevice : public MmeDevice {
 public:
  /**
   * @brief Constructor
   * @param mount_point Path device is mounted to
   * @param name User-friendly device name.
   * @param unique_device_id device unique identifier.
   * @param controller TransportAdapterController observer (MME transport adapter in current implementation)
   */
  IAPDevice(const std::string& mount_point, const std::string& name,
            const DeviceUID& unique_device_id,
            TransportAdapterController* controller);

  /**
   * Destructor
   */
  ~IAPDevice();

  /**
   * @brief Initialize MME device
   * @return \a true on success, \a false otherwise
   */
  virtual bool Init();

  /**
   * @brief Protocol device is connected over, i.e. iAP
   */
  virtual Protocol protocol() const {
    return IAP;
  }

  /**
   * @brief Stop iAP device's internal threads
   */
  virtual void Stop();

 protected:
  virtual ApplicationList GetApplicationList() const;
  virtual void OnConnectionTimeout(const std::string& protocol_name);

 private:
  typedef std::map<uint32_t, ApplicationHandle> AppContainer;
  typedef std::map<int, ApplicationHandle> AppTable;
  typedef std::map<ApplicationHandle, IAPConnection*> ConnectionContainer;
  typedef std::map<int, std::string> FreeProtocolNamePool;
  typedef std::map<std::string, int> ProtocolInUseNamePool;
  typedef std::pair<std::string, ipod_hdl_t*> AppRecord;

  typedef std::map<std::string, ProtocolConnectionTimerSPtr> TimerContainer;

  static const int kProtocolNameSize = 256;

  const AppRecord RegisterConnection(ApplicationHandle app_id,
                                     IAPConnection* connection);
  void UnregisterConnection(ApplicationHandle app_id);
  void OnSessionOpened(uint32_t protocol_id, int session_id);
  void OnSessionOpened(uint32_t protocol_id, const char* protocol_name,
                       int session_id);
  void OnHubSessionOpened(uint32_t protocol_id, const char* protocol_name,
                          int session_id);
  void OnRegularSessionOpened(uint32_t protocol_id, const char* protocol_name,
                              int session_id);
  void OnSessionClosed(int session_id);
  void OnDataReady(int session_id);

  /**
   * Picks protocol from pool of protocols
   * @param index of protocol
   * @param name of protocol
   * @return true if protocol was picked
   */
  bool PickProtocol(char* index, std::string* name);

  /**
   * Frees protocol
   * @param name of protocol
   */
  void FreeProtocol(const std::string& name);

  /**
   * Starts timer for protocol
   * @param name of protocol
   */
  void StartTimer(const std::string& name);

  /**
   * Stops timer for protocol
   * @param name of protocol
   */
  void StopTimer(const std::string& name);

  TransportAdapterController* controller_;
  ipod_hdl_t* ipod_hdl_;
  ApplicationHandle last_app_id_;

  AppContainer apps_;
  mutable sync_primitives::Lock apps_lock_;

  AppTable app_table_;
  sync_primitives::Lock app_table_lock_;

  ConnectionContainer connections_;
  sync_primitives::Lock connections_lock_;

  FreeProtocolNamePool free_protocol_name_pool_;
  ProtocolInUseNamePool protocol_in_use_name_pool_;
  sync_primitives::Lock protocol_name_pool_lock_;
  TimerContainer timers_protocols_;
  sync_primitives::Lock timers_protocols_lock_;

  threads::Thread* receiver_thread_;
  threads::ThreadDelegate* receiver_thread_delegate_;

  /**
   * @brief Thread delegate to receive iAP events
   */
  class IAPEventThreadDelegate : public threads::PulseThreadDelegate {
   public:
    /**
     * @brief Constructor
     * @param ipod_hdl Opaque libipod.so handle
     * @param parent Corresponding iAP device
     */
    IAPEventThreadDelegate(ipod_hdl_t* ipod_hdl, IAPDevice* parent);

   protected:
    virtual bool ArmEvent(struct sigevent* event);
    virtual void OnPulse();

   private:
    static const size_t kEventsBufferSize = 32;

    void ParseEvents();
    void AcceptSession(uint32_t protocol_id);
    void AcceptSession(uint32_t protocol_id, const char* protocol_name);
    void CloseSession(uint32_t session_id);
    void DataReady(uint32_t session_id);
    void OpenSession(uint32_t protocol_id);
    void OpenSession(uint32_t protocol_id, const char* protocol_name);

    IAPDevice* parent_;
    ipod_hdl_t* ipod_hdl_;
    ipod_eaf_event_t events_[kEventsBufferSize];
  };

  friend class IAPConnection;
};

}  // namespace transport_adapter
}  // namespace transport_manager

#endif  // SRC_COMPONENTS_TRANSPORT_MANAGER_INCLUDE_TRANSPORT_MANAGER_IAP_DEVICE_H_
