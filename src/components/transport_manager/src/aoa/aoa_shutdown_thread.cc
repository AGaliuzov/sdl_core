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

#include "transport_manager/aoa/aoa_shutdown_thread.h"
#include "transport_manager/aoa/aoa_wrapper.h"
#include "utils/logger.h"
#include "utils/atomic.h"

namespace transport_manager {
namespace transport_adapter {

CREATE_LOGGERPTR_GLOBAL(logger_, "TransportManager")

AOAShutdownThreadDelegate::AOAShutdownThreadDelegate() : shutdown_(0) {
}

void AOAShutdownThreadDelegate::Shutdown() {
  LOG4CXX_AUTO_TRACE(logger_);
  atomic_post_set(&shutdown_);
  cond_.NotifyOne();
}

void AOAShutdownThreadDelegate::threadMain() {
  sync_primitives::Lock lock;
  run_ = true;
  LOG4CXX_DEBUG(logger_, "Starting AOA shutdown thread");
  while (run_) {
    { // auto_lock scope
      sync_primitives::AutoLock auto_lock(lock);
      LOG4CXX_TRACE(logger_, "Waiting on conditional variable");
      cond_.Wait(auto_lock);
      LOG4CXX_TRACE(logger_, "Got notification on conditional variable");
    } // auto_lock scope
    if (atomic_post_clr(&shutdown_)) {
      AOAWrapper::Shutdown();
    }
  }
  LOG4CXX_DEBUG(logger_, "AOA shutdown thread finished");
}

void AOAShutdownThreadDelegate::exitThreadMain() {
  LOG4CXX_AUTO_TRACE(logger_);
  run_ = false;
  cond_.NotifyOne();
}

}  // namespace transport_adapter
}  // namespace transport_manager
