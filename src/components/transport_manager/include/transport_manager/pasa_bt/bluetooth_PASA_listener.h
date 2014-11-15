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
#ifndef SRC_COMPONENTS_TRANSPORT_MANAGER_INCLUDE_TRANSPORT_MANAGER_PASA_BT_BLUETOOTH_PASA_LISTENER_H_
#define SRC_COMPONENTS_TRANSPORT_MANAGER_INCLUDE_TRANSPORT_MANAGER_PASA_BT_BLUETOOTH_PASA_LISTENER_H_

#include <mqueue.h>
#include <applink_types.h>

#include "utils/threads/thread_delegate.h"
#include "transport_manager/transport_adapter/client_connection_listener.h"

class Thread;

namespace transport_manager {
namespace transport_adapter {

class TransportAdapterController;

/**
 * @brief Listening to applink service to connected devices (applications)
 */
class BluetoothPASAListener : public ClientConnectionListener {
 public:
  /**
   * @breaf Constructor.
   * @param controller Pointer to the device adapter controller.
   */
  explicit BluetoothPASAListener(TransportAdapterController* controller);

  /**
   * @brief Destructor.
   */
  virtual ~BluetoothPASAListener();

  /**
   * @brief Initializes Bluetooth PASA client listener
   *
   * @return Error information about possible reason of starting listener failure.
   */
  virtual TransportAdapter::Error Init();

  /**
   * @brief Terminates Bluetooth PASA client listener.
   */
  virtual void Terminate();

  /**
   * @brief Check initialization.
   *
   * @return True if initialized.
   * @return False if not initialized.
   */
  virtual bool IsInitialised() const;

  /**
   * @brief Starts listening
   * @return Error information about possible reason of failure.
   */
  virtual TransportAdapter::Error StartListening();

  /**
   * @brief Stops listening
   */
  virtual TransportAdapter::Error StopListening();

 private:
  TransportAdapterController* controller_;
  threads::Thread* thread_;
  mqd_t mq_from_sdl_;
  mqd_t mq_to_sdl_;

  void Loop();

  /**
   * @brief Connect BT device
   * Called on PASA FW BT SPP Connect Message
   */
  void ConnectDevice(void* data);

  /**
   * @brief Disconnect BT device
   * Called on PASA FW BT Disconnect Message
   */
  void DisconnectDevice(void* data);
  /**
   * @brief Disconnect SPP (close connection)
   * Called on PPASA FW BT SPP Disconnect Message
   */
  void DisconnectSPP(void* data);

  /**
   * @brief Summarizes the total list of applications and notifies controller
   * Called on PASA FW BT SPP END Message
   */
  void UpdateTotalApplicationList();

  class ListeningThreadDelegate : public threads::ThreadDelegate {
   public:
    explicit ListeningThreadDelegate(BluetoothPASAListener* parent);
    virtual void threadMain();
   private:
    BluetoothPASAListener* parent_;
  };
};

}  // namespace transport_adapter
}  // namespace transport_manager

#endif  // SRC_COMPONENTS_TRANSPORT_MANAGER_INCLUDE_TRANSPORT_MANAGER_PASA_BT_BLUETOOTH_PASA_LISTENER_H_
