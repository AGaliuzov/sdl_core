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

#ifndef SRC_COMPONENTS_APPLICATION_MANAGER_INCLUDE_APPLICATION_MANAGER_H_
#define SRC_COMPONENTS_APPLICATION_MANAGER_INCLUDE_APPLICATION_MANAGER_H_
#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include <stdint.h>
#include <vector>
#include <map>
#include <set>
#include "application_manager/hmi_command_factory.h"
#include "application_manager/application_manager.h"
#include "application_manager/hmi_capabilities.h"
#include "application_manager/message.h"
#include "application_manager/request_controller.h"
#include "application_manager/resume_ctrl.h"
#include "application_manager/vehicle_info_data.h"
#include "protocol_handler/protocol_observer.h"
#include "hmi_message_handler/hmi_message_observer.h"
#include "hmi_message_handler/hmi_message_sender.h"

#include "media_manager/media_manager_impl.h"

#include "connection_handler/connection_handler_observer.h"
#include "connection_handler/device.h"

#include "formatters/CSmartFactory.hpp"

#include "interfaces/HMI_API.h"
#include "interfaces/HMI_API_schema.h"
#include "interfaces/MOBILE_API_schema.h"

#include "interfaces/v4_protocol_v1_2_no_extra.h"
#include "interfaces/v4_protocol_v1_2_no_extra_schema.h"
#ifdef TIME_TESTER
#include "time_metric_observer.h"
#endif  // TIME_TESTER

#include "utils/macro.h"
#include "utils/shared_ptr.h"
#include "utils/message_queue.h"
#include "utils/prioritized_queue.h"
#include "utils/threads/thread.h"
#include "utils/threads/message_loop_thread.h"
#include "utils/lock.h"
#include "utils/singleton.h"

namespace application_manager {

namespace impl {

struct MessageFromMobile: public utils::SharedPtr<Message> {
  explicit MessageFromMobile(const utils::SharedPtr<Message>& message)
      : utils::SharedPtr<Message>(message) {
  }
  // PrioritizedQueue requres this method to decide which priority to assign
  size_t PriorityOrder() const {
    return (*this)->Priority().OrderingValue();
  }
};

struct MessageToMobile: public utils::SharedPtr<Message> {
  explicit MessageToMobile(const utils::SharedPtr<Message>& message,
                           bool final_message)
      : utils::SharedPtr<Message>(message),
        is_final(final_message) {
  }
  // PrioritizedQueue requres this method to decide which priority to assign
  size_t PriorityOrder() const {
    return (*this)->Priority().OrderingValue();
  }
  // Signals if connection to mobile must be closed after sending this message
  bool is_final;
};

struct MessageFromHmi: public utils::SharedPtr<Message> {
  explicit MessageFromHmi(const utils::SharedPtr<Message>& message)
      : utils::SharedPtr<Message>(message) {
  }
  // PrioritizedQueue requres this method to decide which priority to assign
  size_t PriorityOrder() const {
    return (*this)->Priority().OrderingValue();
  }
};

struct MessageToHmi: public utils::SharedPtr<Message> {
  explicit MessageToHmi(const utils::SharedPtr<Message>& message)
      : utils::SharedPtr<Message>(message) {
  }
  // PrioritizedQueue requres this method to decide which priority to assign
  size_t PriorityOrder() const {
    return (*this)->Priority().OrderingValue();
  }
};

typedef threads::MessageLoopThread<utils::PrioritizedQueue<MessageFromMobile> > FromMobileQueue;
typedef threads::MessageLoopThread<utils::PrioritizedQueue<MessageToMobile> > ToMobileQueue;
typedef threads::MessageLoopThread<utils::PrioritizedQueue<MessageFromHmi> > FromHmiQueue;
typedef threads::MessageLoopThread<utils::PrioritizedQueue<MessageToHmi> > ToHmiQueue;
}

class ApplicationManagerImpl : public ApplicationManager,
  public hmi_message_handler::HMIMessageObserver,
  public protocol_handler::ProtocolObserver,
  public connection_handler::ConnectionHandlerObserver,
  public impl::FromMobileQueue::Handler, public impl::ToMobileQueue::Handler,
  public impl::FromHmiQueue::Handler, public impl::ToHmiQueue::Handler,
  public utils::Singleton<ApplicationManagerImpl> {

    friend class ResumeCtrl;
    friend class CommandImpl;

 public:
  MOCK_METHOD0(Init, bool());
  MOCK_METHOD0(Stop, bool());
  MOCK_METHOD1(OnMessageReceived, void (utils::SharedPtr<application_manager::Message>));

  MOCK_METHOD1(OnErrorSending, void (utils::SharedPtr<application_manager::Message>));
  MOCK_METHOD1(OnMessageReceived, void (const RawMessagePtr));
  MOCK_METHOD1(OnMobileMessageSent, void (const RawMessagePtr));

  MOCK_METHOD1(OnDeviceListUpdated, void (const connection_handler::DeviceMap&));
  MOCK_METHOD0(OnFindNewApplicationsRequest, void ());
  MOCK_METHOD1(RemoveDevice, void (const connection_handler::DeviceHandle&));
  MOCK_METHOD3(OnServiceStartedCallback, bool (const connection_handler::DeviceHandle&,
                                            const int32_t&,
                                            const protocol_handler::ServiceType&));
  MOCK_METHOD2(OnServiceEndedCallback, void (const int32_t&,
                                             const protocol_handler::ServiceType&));
  MOCK_METHOD1(Handle, void (const impl::MessageFromMobile));
  MOCK_METHOD1(Handle, void (const impl::MessageToMobile));
  MOCK_METHOD1(Handle, void (const impl::MessageFromHmi));
  MOCK_METHOD1(Handle, void (const impl::MessageToHmi));
  MOCK_METHOD1(set_hmi_message_handler, void (hmi_message_handler::HMIMessageHandler*));
  MOCK_METHOD1(set_protocol_handler, void (protocol_handler::ProtocolHandler*));
  MOCK_METHOD1(set_connection_handler, void (connection_handler::ConnectionHandler*));

  FRIEND_BASE_SINGLETON_CLASS(ApplicationManagerImpl);
};

} //application_manager
#endif  // SRC_COMPONENTS_APPLICATION_MANAGER_INCLUDE_APPLICATION_MANAGER_H_
