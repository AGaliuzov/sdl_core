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

#ifndef SRC_COMPONENTS_TRANSPORT_MANAGER_INCLUDE_TRANSPORT_MANAGER_IAP2_DEVICE_H_
#define SRC_COMPONENTS_TRANSPORT_MANAGER_INCLUDE_TRANSPORT_MANAGER_IAP2_DEVICE_H_

#include <map>
#include <list>
#include <string>
#include <iap2/iap2.h>

#include "transport_manager/mme/mme_device.h"
#include "transport_manager/mme/protocol_connection_timer.h"
#include "transport_manager/transport_adapter/transport_adapter_controller.h"
#include "utils/shared_ptr.h"
#include "utils/threads/thread_delegate.h"
#include "utils/threads/thread.h"
#include "utils/lock.h"
#include "utils/timer_thread.h"

namespace transport_manager {
namespace transport_adapter {

/**
 * @brief Class representing device connected over iAP2 protocol
 */
class IAP2Device : public MmeDevice {
 public:
  /**
   * @brief Constructor
   * @param mount_point Path device is mounted to
   * @param name User-friendly device name.
   * @param unique_device_id device unique identifier.
   * @param controller TransportAdapterController observer (MME transport adapter in current implementation)
   */
  IAP2Device(const std::string& mount_point, const std::string& name,
             const DeviceUID& unique_device_id,
             TransportAdapterController* controller);

  /**
   * Destructor
   */
  ~IAP2Device();

  /**
   * @brief Initialize MME device
   * @return \a true on success, \a false otherwise
   */
  virtual bool Init();

  /**
   * @brief Protocol device is connected over, i.e. iAP2
   */
  virtual Protocol protocol() const {
    return IAP2;
  }

  /**
   * @brief Stop iAP2 device's internal threads
   */
  virtual void Stop();

 protected:
  virtual ApplicationList GetApplicationList() const;
  virtual void OnConnectionTimeout(const std::string& protocol_name);

 private:
  typedef std::map<int, std::string> FreeProtocolNamePool;
  typedef std::map<std::string, int> ProtocolInUseNamePool;
  struct AppRecord;
  typedef std::map<ApplicationHandle, AppRecord> AppContainer;
  typedef std::map<std::string, threads::Thread*> ThreadContainer;

  typedef std::map<std::string, ProtocolConnectionTimerSPtr> TimerContainer;

  bool RecordByAppId(ApplicationHandle app_id, AppRecord& record) const;

  void OnHubConnect(const std::string& protocol_name, iap2ea_hdl_t* handle);
  void OnConnect(const std::string& protocol_name, iap2ea_hdl_t* handle);
  void OnConnectFailed(const std::string& protocol_name);
  void OnDisconnect(ApplicationHandle app_id);

  /**
   * Starts thread for protocol name
   * @param name
   */
  void StartThread(const std::string& protocol_name);

  /**
   * Stops thread for protocol name
   * @param name
   */
  void StopThread(const std::string& protocol_name);

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
  bool FreeProtocol(const std::string& name);

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

  void KillFinishedConnections();

  TransportAdapterController* controller_;
  int last_app_id_;

  AppContainer apps_;
  mutable sync_primitives::Lock apps_lock_;

  FreeProtocolNamePool free_protocol_name_pool_;
  ProtocolInUseNamePool protocol_in_use_name_pool_;
  sync_primitives::Lock protocol_name_pool_lock_;

  ThreadContainer legacy_connection_threads_;
  ThreadContainer hub_connection_threads_;
  ThreadContainer pool_connection_threads_;
  sync_primitives::Lock pool_connection_threads_lock_;
  TimerContainer timers_protocols_;
  sync_primitives::Lock timers_protocols_lock_;

  struct AppRecord {
    std::string protocol_name;
    iap2ea_hdl_t* handle;
    bool to_remove;
  };

  /**
   * @brief Thread delegate to wait for connection on hub protocol
   */
  class IAP2HubConnectThreadDelegate : public threads::ThreadDelegate {
   public:
    /**
     * @brief Constructor
     * @param parent Corresponding iAP2 device
     * @param protocol_name Hub protocol to connect
     */
    IAP2HubConnectThreadDelegate(IAP2Device* parent,
                                 const std::string& protocol_name);
    /**
     * @brief Thread procedure.
     * (endless loop blocking on waiting for connection on specified protocol
     * and invoking parent's callback on connection)
     */
    void threadMain();
   private:
    IAP2Device* parent_;
    std::string protocol_name_;
  };

  /**
   * @brief Thread delegate to wait for connection on pool protocol
   */
  class IAP2ConnectThreadDelegate : public threads::ThreadDelegate {
   public:
    /**
     * @brief Constructor
     * @param parent Corresponding iAP2 device
     * @param protocol_name Pool protocol to connect
     */
    IAP2ConnectThreadDelegate(IAP2Device* parent,
                              const std::string& protocol_name);
    /**
     * @brief Thread procedure.
     * (blocking once on waiting for connection on specified protocol
     * and invoking parent's callback on connection)
     */
    void threadMain();
   private:
    IAP2Device* parent_;
    std::string protocol_name_;
  };

  friend class IAP2Connection;
};

}  // namespace transport_adapter
}  // namespace transport_manager

#endif  // SRC_COMPONENTS_TRANSPORT_MANAGER_INCLUDE_TRANSPORT_MANAGER_IAP2_DEVICE_H_
