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

#ifndef SRC_COMPONENTS_TRANSPORT_MANAGER_INCLUDE_TRANSPORT_MANAGER_MME_PROTOCOL_CONNECTION_TIMER_H_
#define SRC_COMPONENTS_TRANSPORT_MANAGER_INCLUDE_TRANSPORT_MANAGER_MME_PROTOCOL_CONNECTION_TIMER_H_

#include <string>

#include "utils/timer_thread.h"
#include "utils/shared_ptr.h"
#include "transport_manager/mme/mme_device.h"

namespace transport_manager {
namespace transport_adapter {

/**
 * @brief Timer to make sure that connection on protocol is established
 */
class ProtocolConnectionTimer {
 public:
  /**
   * @brief Constructor
   * @param protocol_name Pool protocol to take care of
   * @param parent Corresponding MME device
   */
  ProtocolConnectionTimer(const std::string& protocol_name, MmeDevice* parent);
  /**
   * Destructor
   */
  ~ProtocolConnectionTimer();
  /**
   * @brief Start timer
   */
  void Start();
  /**
   * @brief Stop timer
   */
  void Stop();

 private:
  typedef timer::TimerThread<ProtocolConnectionTimer> Timer;
  Timer* timer_;
  std::string protocol_name_;
  MmeDevice* parent_;

  void Shoot();
};

typedef utils::SharedPtr<ProtocolConnectionTimer> ProtocolConnectionTimerSPtr;

}  // namespace transport_adapter
}  // namespace transport_manager

#endif  // SRC_COMPONENTS_TRANSPORT_MANAGER_INCLUDE_TRANSPORT_MANAGER_MME_PROTOCOL_CONNECTION_TIMER_H_
