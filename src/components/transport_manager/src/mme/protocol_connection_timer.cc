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

#include "transport_manager/mme/protocol_connection_timer.h"

#include "config_profile/profile.h"
#include "utils/timer_task_impl.h"

namespace transport_manager {
namespace transport_adapter {

CREATE_LOGGERPTR_GLOBAL(logger_, "TransportManager")

ProtocolConnectionTimer::ProtocolConnectionTimer(
    const std::string& protocol_name, MmeDevice* parent)
    : timer_("proto releaser",
             new timer::TimerTaskImpl<ProtocolConnectionTimer>(
                 this, &ProtocolConnectionTimer::Shoot))
    , protocol_name_(protocol_name)
    , parent_(parent) {}

ProtocolConnectionTimer::~ProtocolConnectionTimer() {
  Stop();
}

void ProtocolConnectionTimer::Start() {
  int timeout = profile::Profile::instance()->iap_hub_connection_wait_timeout();
  LOG4CXX_DEBUG(logger_,
                "Starting timer for protocol " << protocol_name_ << " ("
                                               << parent_->protocol()
                                               << ")"
                                                  " with timeout " << timeout);
  timer_.Start(timeout, false);
}

void ProtocolConnectionTimer::Stop() {                                                                                                                                                                                                                  
  LOG4CXX_DEBUG(logger_,
                "Stopping timer for protocol " << protocol_name_ << " ("
                                               << parent_->protocol() << ")");
  timer_.Stop();
}

void ProtocolConnectionTimer::Shoot() {
  LOG4CXX_INFO(logger_,
               "Connection timeout for protocol "
                   << protocol_name_ << " (" << parent_->protocol() << ")");
  parent_->OnConnectionTimeout(protocol_name_);
}

}  // namespace transport_adapter
}  // namespace transport_manager
