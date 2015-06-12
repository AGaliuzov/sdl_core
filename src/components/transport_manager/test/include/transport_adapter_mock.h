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

#ifndef SRC_COMPONENTS_TRANSPORT_MANAGER_TEST_INCLUDE_TRANSPORT_ADAPTER_MOCK_H_
#define SRC_COMPONENTS_TRANSPORT_MANAGER_TEST_INCLUDE_TRANSPORT_ADAPTER_MOCK_H_

#include "gmock/gmock.h"
#include "transport_manager/transport_adapter/transport_adapter.h"

namespace test {
namespace components {
namespace transport_manager {

using namespace ::transport_manager::transport_adapter;

class TransportAdapterMock : public TransportAdapter {
 public:
  MOCK_CONST_METHOD0(GetDeviceType, DeviceType());
  MOCK_CONST_METHOD0(GetConnectionType, ConnectionType());
  MOCK_CONST_METHOD0(IsInitialised, bool());
  MOCK_METHOD0(Init, Error());
  MOCK_METHOD0(Terminate, void());
  MOCK_METHOD1(AddListener, void(TransportAdapterListener* listener));
  MOCK_CONST_METHOD0(IsSearchDevicesSupported, bool());
  MOCK_METHOD0(SearchDevices, Error());
  MOCK_CONST_METHOD0(IsServerOriginatedConnectSupported, bool());
  MOCK_METHOD2(Connect, Error(const DeviceUID& device_handle,
                              const ApplicationHandle& app_handle));
  MOCK_METHOD1(ConnectDevice, Error(const DeviceUID& device_handle));
  MOCK_CONST_METHOD0(IsClientOriginatedConnectSupported, bool());
  MOCK_METHOD0(StartClientListening, Error());
  MOCK_METHOD0(StopClientListening, Error());
  MOCK_METHOD2(Disconnect, Error(const DeviceUID& device_handle,
                              const ApplicationHandle& app_handle));
  MOCK_METHOD1(DisconnectDevice, Error(const DeviceUID& device_handle));
  MOCK_METHOD3(SendData, Error(const DeviceUID& device_handle,
                               const ApplicationHandle& app_handle,
                               const protocol_handler::RawMessagePtr data));
  MOCK_CONST_METHOD0(GetDeviceList, DeviceList());
  MOCK_CONST_METHOD1(GetApplicationList, ApplicationList(const DeviceUID& device_handle));
  MOCK_CONST_METHOD1(DeviceName, std::string(const DeviceUID& device_handle));

#ifdef TIME_TESTER
  MOCK_METHOD0(GetTimeMetricObserver, TMMetricObserver*());
#endif // TIME_TESTER
};

}  // namespace transport_manager
}  // namespace components
}  // namespace test

#endif // SRC_COMPONENTS_TRANSPORT_MANAGER_TEST_INCLUDE_TRANSPORT_ADAPTER_MOCK_H_
