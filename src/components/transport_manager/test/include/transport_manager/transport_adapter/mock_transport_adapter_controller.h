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

#ifndef SRC_COMPONENTS_TRANSPORT_MANAGER_TEST_INCLUDE_TRANSPORT_MANAGER_TRANSPORT_ADAPTER_MOCK_TRANSPORT_ADAPTER_CONTROLLER_H_
#define SRC_COMPONENTS_TRANSPORT_MANAGER_TEST_INCLUDE_TRANSPORT_MANAGER_TRANSPORT_ADAPTER_MOCK_TRANSPORT_ADAPTER_CONTROLLER_H_

#include "gmock/gmock.h"
#include "transport_manager/transport_adapter/transport_adapter_controller.h"
#include "protocol/raw_message.h"

namespace test {
namespace components {
namespace transport_manager_test {

using ::transport_manager::transport_adapter::DeviceVector;
using ::transport_manager::transport_adapter::DeviceSptr;
using ::transport_manager::transport_adapter::ConnectionSPtr;
using ::transport_manager::DeviceUID;
using ::transport_manager::ApplicationHandle;
using ::transport_manager::ConnectError;

class MockTransportAdapterController
    : public ::transport_manager::transport_adapter::
          TransportAdapterController {
 public:
  MOCK_METHOD1(AddDevice, DeviceSptr(DeviceSptr device));
  MOCK_METHOD1(SearchDeviceDone, void(const DeviceVector& devices));
  MOCK_METHOD1(ApplicationListUpdated, void(const DeviceUID& device_handle));
  MOCK_METHOD0(FindNewApplicationsRequest, void());
  MOCK_METHOD1(SearchDeviceFailed,
               void(const ::transport_manager::SearchDeviceError& error));
  MOCK_CONST_METHOD1(FindDevice, DeviceSptr(const DeviceUID& device_handle));
  MOCK_METHOD0(AckDevices, void());
  MOCK_METHOD3(ConnectionCreated,
               void(ConnectionSPtr connection, const DeviceUID& device_handle,
                    const ApplicationHandle& app_handle));
  MOCK_METHOD2(ConnectDone, void(const DeviceUID& device_handle,
                                 const ApplicationHandle& app_handle));
  MOCK_METHOD3(ConnectFailed,
               void(const DeviceUID& device_handle,
                    const ApplicationHandle& app_handle,
                    const ::transport_manager::ConnectError& error));
  MOCK_METHOD2(ConnectionFinished, void(const DeviceUID& device_handle,
                                        const ApplicationHandle& app_handle));
  MOCK_METHOD3(ConnectionAborted,
               void(const DeviceUID& device_handle,
                    const ApplicationHandle& app_handle,
                    const ::transport_manager::CommunicationError& error));
  MOCK_METHOD2(DeviceDisconnected,
               void(const DeviceUID& device_handle,
                    const ::transport_manager::DisconnectDeviceError& error));
  MOCK_METHOD2(DisconnectDone, void(const DeviceUID& device_handle,
                                    const ApplicationHandle& app_handle));
  MOCK_METHOD3(DataReceiveDone,
               void(const DeviceUID& device_handle,
                    const ApplicationHandle& app_handle,
                    ::protocol_handler::RawMessagePtr message));
  MOCK_METHOD3(DataReceiveFailed,
               void(const DeviceUID& device_handle,
                    const ApplicationHandle& app_handle,
                    const ::transport_manager::DataReceiveError&));
  MOCK_METHOD4(DataSendFailed,
               void(const DeviceUID& device_handle,
                    const ApplicationHandle& app_handle,
                    const ::protocol_handler::RawMessagePtr message,
                    const ::transport_manager::DataSendError& error));

  MOCK_METHOD3(DataSendDone, void(const DeviceUID& device_handle,
                                  const ApplicationHandle& app_handle,
                                  ::protocol_handler::RawMessagePtr message));
};

}  // namespace transport_manager_test
}  // namespace components
}  // namespace test

#endif  // SRC_COMPONENTS_TRANSPORT_MANAGER_TEST_INCLUDE_TRANSPORT_MANAGER_TRANSPORT_ADAPTER_MOCK_TRANSPORT_ADAPTER_CONTROLLER_H_