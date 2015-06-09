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

#include "gtest/gtest.h"
#include "include/device_scanner_mock.h"
#include "include/client_connection_listener_mock.h"
#include "include/server_connection_factory_mock.h"
#include "include/device_mock.h"
#include "include/connection_mock.h"
#include "include/transport_adapter_listener_mock.h"

#include "transport_manager/transport_adapter/transport_adapter_impl.h"
#include "transport_manager/transport_adapter/transport_adapter_listener.h"
#include "transport_manager/transport_adapter/transport_adapter_controller.h"
#include "transport_manager/transport_adapter/connection.h"
#include "protocol/raw_message.h"

namespace test {
namespace components {
namespace transport_manager {

using ::testing::Return;
using ::testing::SetArgReferee;
using ::testing::_;

class TestTransportAdapter : public TransportAdapterImpl {
 public:
  TestTransportAdapter(DeviceScanner* device_scanner,
                       ServerConnectionFactory* server_connection_factory,
                       ClientConnectionListener* client_connection_listener)
      : TransportAdapterImpl(device_scanner, server_connection_factory,
                             client_connection_listener) {}

  ConnectionSPtr FindEstabledConnection(const DeviceUID& device_handle,
                                        const ApplicationHandle& app_handle) {
    return this->FindEstablishedConnection(device_handle, app_handle);
  }
  virtual ~TestTransportAdapter(){};

 protected:
  virtual DeviceType GetDeviceType() const { return UNKNOWN; }

  virtual void Store() const {}

  virtual bool Restore() { return true; }
};

TEST(TransportAdapterTest, Init) {
  DeviceScannerMock* dev_mock = new DeviceScannerMock();
  ClientConnectionListenerMock* clientMock = new ClientConnectionListenerMock();
  ServerConnectionFactoryMock* serverMock = new ServerConnectionFactoryMock();
  TestTransportAdapter transport_adapter(dev_mock, serverMock, clientMock);

  EXPECT_CALL(*dev_mock, Init()).WillOnce(Return(TransportAdapter::OK));
  EXPECT_CALL(*clientMock, Init()).WillOnce(Return(TransportAdapter::OK));
  EXPECT_CALL(*serverMock, Init()).WillOnce(Return(TransportAdapter::OK));
  EXPECT_EQ(TransportAdapter::OK, transport_adapter.Init());

  EXPECT_CALL(*dev_mock, Terminate());
  EXPECT_CALL(*clientMock, Terminate());
  EXPECT_CALL(*serverMock, Terminate());
}

TEST(TransportAdapterTest, SearchDevices_WithoutScanner) {
  ClientConnectionListenerMock* clientMock = new ClientConnectionListenerMock();
  ServerConnectionFactoryMock* serverMock = new ServerConnectionFactoryMock();
  TestTransportAdapter transport_adapter(NULL, serverMock, clientMock);

  EXPECT_CALL(*clientMock, Init()).WillOnce(Return(TransportAdapter::OK));
  EXPECT_CALL(*serverMock, Init()).WillOnce(Return(TransportAdapter::OK));
  EXPECT_EQ(TransportAdapter::OK, transport_adapter.Init());

  EXPECT_EQ(TransportAdapter::NOT_SUPPORTED, transport_adapter.SearchDevices());

  EXPECT_CALL(*clientMock, Terminate());
  EXPECT_CALL(*serverMock, Terminate());
}

TEST(TransportAdapterTest, SearchDevices_DeviceNotInitialized) {
  DeviceScannerMock* dev_mock = new DeviceScannerMock();
  TestTransportAdapter transport_adapter(dev_mock, NULL, NULL);

  EXPECT_CALL(*dev_mock, Init()).WillOnce(Return(TransportAdapter::OK));
  transport_adapter.Init();

  EXPECT_CALL(*dev_mock, IsInitialised()).WillRepeatedly(Return(false));
  EXPECT_CALL(*dev_mock, Scan()).Times(0);
  EXPECT_EQ(TransportAdapter::BAD_STATE, transport_adapter.SearchDevices());
  EXPECT_CALL(*dev_mock, Terminate());
}

TEST(TransportAdapterTest, SearchDevices_DeviceInitialized) {
  DeviceScannerMock* dev_mock = new DeviceScannerMock();
  TestTransportAdapter transport_adapter(dev_mock, NULL, NULL);

  EXPECT_CALL(*dev_mock, Init()).WillOnce(Return(TransportAdapter::OK));
  transport_adapter.Init();

  EXPECT_CALL(*dev_mock, IsInitialised()).WillOnce(Return(true));
  EXPECT_CALL(*dev_mock, Scan()).WillRepeatedly(Return(TransportAdapter::OK));
  EXPECT_EQ(TransportAdapter::OK, transport_adapter.SearchDevices());

  EXPECT_CALL(*dev_mock, Terminate());
}

TEST(TransportAdapterTest, SearchDeviceDone_DeviceExisting) {
  TestTransportAdapter transport_adapter(NULL, NULL, NULL);
  transport_adapter.Init();

  std::string dev_id = "device_id";
  std::string uniq_id = "unique_device_id";

  utils::SharedPtr<DeviceMock> mockdev = new DeviceMock(dev_id, uniq_id);
  transport_adapter.AddDevice(mockdev);

  std::vector<utils::SharedPtr<Device>> devList;
  devList.push_back(mockdev);

  EXPECT_CALL(*mockdev, IsSameAs(_)).WillOnce(Return(true));
  transport_adapter.SearchDeviceDone(devList);
}

TEST(TransportAdapterTest, SearchDeviceFailed) {
  TestTransportAdapter transport_adapter(NULL, NULL, NULL);
  transport_adapter.Init();

  TransportAdapterListenerMock mock_listener;
  transport_adapter.AddListener(&mock_listener);

  SearchDeviceError er;
  EXPECT_CALL(mock_listener, OnSearchDeviceFailed(_, _));
  transport_adapter.SearchDeviceFailed(er);
}

TEST(TransportAdapterTest, AddDevice) {
  TestTransportAdapter transport_adapter(NULL, NULL, NULL);
  transport_adapter.Init();

  std::string dev_id = "device_id";
  std::string uniq_id = "unique_device_id";

  TransportAdapterListenerMock mock_listener;
  transport_adapter.AddListener(&mock_listener);

  DeviceMock* mockdev = new DeviceMock(dev_id, uniq_id);

  EXPECT_CALL(mock_listener, OnDeviceListUpdated(_));
  transport_adapter.AddDevice(mockdev);
}

TEST(TransportAdapterTest, Connect_ServerNotSupported) {
  ClientConnectionListenerMock* clientMock = new ClientConnectionListenerMock();
  TestTransportAdapter transport_adapter(NULL, NULL, clientMock);

  EXPECT_CALL(*clientMock, Init()).WillOnce(Return(TransportAdapter::OK));
  transport_adapter.Init();

  int app_handle = 1;
  std::string dev_id = "device_id";
  TransportAdapter::Error res = transport_adapter.Connect(dev_id, app_handle);
  EXPECT_EQ(TransportAdapter::NOT_SUPPORTED, res);

  EXPECT_CALL(*clientMock, Terminate());
}

TEST(TransportAdapterTest, Connect_ServerNotInitialized) {
  ServerConnectionFactoryMock* serverMock = new ServerConnectionFactoryMock();
  TestTransportAdapter transport_adapter(NULL, serverMock, NULL);

  EXPECT_CALL(*serverMock, Init()).WillOnce(Return(TransportAdapter::OK));
  transport_adapter.Init();

  int app_handle = 1;
  std::string dev_id = "device_id";

  EXPECT_CALL(*serverMock, IsInitialised()).WillOnce(Return(false));
  EXPECT_CALL(*serverMock, CreateConnection(dev_id, app_handle)).Times(0);
  TransportAdapter::Error res = transport_adapter.Connect(dev_id, app_handle);
  EXPECT_EQ(TransportAdapter::BAD_STATE, res);

  EXPECT_CALL(*serverMock, Terminate());
}

TEST(TransportAdapterTest, Connect_Success) {
  ServerConnectionFactoryMock* serverMock = new ServerConnectionFactoryMock();
  TestTransportAdapter transport_adapter(NULL, serverMock, NULL);

  EXPECT_CALL(*serverMock, Init()).WillOnce(Return(TransportAdapter::OK));
  transport_adapter.Init();

  int app_handle = 1;
  std::string dev_id = "device_id";

  EXPECT_CALL(*serverMock, IsInitialised()).WillOnce(Return(true));
  EXPECT_CALL(*serverMock, CreateConnection(dev_id, app_handle))
      .WillOnce(Return(TransportAdapter::OK));
  TransportAdapter::Error res = transport_adapter.Connect(dev_id, app_handle);
  EXPECT_EQ(TransportAdapter::OK, res);

  EXPECT_CALL(*serverMock, Terminate());
}

TEST(TransportAdapterTest, Connect_DeviceAddedTwice) {
  ServerConnectionFactoryMock* serverMock = new ServerConnectionFactoryMock();
  TestTransportAdapter transport_adapter(NULL, serverMock, NULL);

  EXPECT_CALL(*serverMock, Init()).WillOnce(Return(TransportAdapter::OK));
  transport_adapter.Init();

  int app_handle = 1;
  std::string dev_id = "device_id";

  EXPECT_CALL(*serverMock, IsInitialised()).WillOnce(Return(true));
  EXPECT_CALL(*serverMock, CreateConnection(dev_id, app_handle))
      .WillOnce(Return(TransportAdapter::OK));
  TransportAdapter::Error res = transport_adapter.Connect(dev_id, app_handle);
  EXPECT_EQ(TransportAdapter::OK, res);

  EXPECT_CALL(*serverMock, IsInitialised()).WillOnce(Return(true));
  TransportAdapter::Error newres =
      transport_adapter.Connect(dev_id, app_handle);
  EXPECT_EQ(TransportAdapter::ALREADY_EXISTS, newres);

  EXPECT_CALL(*serverMock, Terminate());
}

TEST(TransportAdapterTest, ConnectDevice_ServerNotAdded_DeviceAdded) {
  TestTransportAdapter transport_adapter(NULL, NULL, NULL);
  transport_adapter.Init();

  std::string dev_id = "device_id";
  std::string uniq_id = "unique_device_id";

  DeviceMock* mockdev = new DeviceMock(dev_id, uniq_id);
  transport_adapter.AddDevice(mockdev);

  std::vector<std::string> devList = transport_adapter.GetDeviceList();
  EXPECT_EQ(1, devList.size());
  if (devList.size() == 1) {
    EXPECT_EQ(uniq_id, devList[0]);
  }

  int app_handle = 1;
  std::vector<int> intList = {app_handle};
  EXPECT_CALL(*mockdev, GetApplicationList()).WillOnce(Return(intList));

  TransportAdapter::Error res = transport_adapter.ConnectDevice(uniq_id);
  EXPECT_EQ(TransportAdapter::FAIL, res);
}

TEST(TransportAdapterTest, ConnectDevice_DeviceNotAdded) {
  ServerConnectionFactoryMock* serverMock = new ServerConnectionFactoryMock();
  TestTransportAdapter transport_adapter(NULL, serverMock, NULL);

  EXPECT_CALL(*serverMock, Init()).WillOnce(Return(TransportAdapter::OK));
  transport_adapter.Init();

  std::string uniq_id = "unique_device_id";

  EXPECT_CALL(*serverMock, IsInitialised()).Times(0);
  EXPECT_CALL(*serverMock, CreateConnection(_, _)).Times(0);
  TransportAdapter::Error res = transport_adapter.ConnectDevice(uniq_id);
  EXPECT_EQ(TransportAdapter::BAD_PARAM, res);

  EXPECT_CALL(*serverMock, Terminate());
}

TEST(TransportAdapterTest, ConnectDevice_DeviceAdded) {
  ServerConnectionFactoryMock* serverMock = new ServerConnectionFactoryMock();
  TestTransportAdapter transport_adapter(NULL, serverMock, NULL);

  EXPECT_CALL(*serverMock, Init()).WillOnce(Return(TransportAdapter::OK));
  transport_adapter.Init();

  std::string dev_id = "device_id";
  std::string uniq_id = "unique_device_id";

  DeviceMock* mockdev = new DeviceMock(dev_id, uniq_id);
  transport_adapter.AddDevice(mockdev);

  std::vector<std::string> devList = transport_adapter.GetDeviceList();
  EXPECT_EQ(1, devList.size());
  if (devList.size() == 1) {
    EXPECT_EQ(uniq_id, devList[0]);
  }

  int app_handle = 1;
  std::vector<int> intList = {app_handle};
  EXPECT_CALL(*mockdev, GetApplicationList()).WillOnce(Return(intList));

  EXPECT_CALL(*serverMock, IsInitialised()).WillOnce(Return(true));
  EXPECT_CALL(*serverMock, CreateConnection(uniq_id, app_handle))
      .WillOnce(Return(TransportAdapter::OK));
  TransportAdapter::Error res = transport_adapter.ConnectDevice(uniq_id);
  EXPECT_EQ(TransportAdapter::OK, res);

  EXPECT_CALL(*serverMock, Terminate());
}

TEST(TransportAdapterTest, ConnectDevice_DeviceAddedTwice) {
  ServerConnectionFactoryMock* serverMock = new ServerConnectionFactoryMock();
  TestTransportAdapter transport_adapter(NULL, serverMock, NULL);

  EXPECT_CALL(*serverMock, Init()).WillOnce(Return(TransportAdapter::OK));
  transport_adapter.Init();

  std::string dev_id = "device_id";
  std::string uniq_id = "unique_device_id";
  ;

  DeviceMock* mockdev = new DeviceMock(dev_id, uniq_id);
  transport_adapter.AddDevice(mockdev);

  std::vector<std::string> devList = transport_adapter.GetDeviceList();
  EXPECT_EQ(1, devList.size());
  if (devList.size() == 1) {
    EXPECT_EQ(uniq_id, devList[0]);
  }

  int app_handle = 1;
  std::vector<int> intList = {app_handle};
  EXPECT_CALL(*mockdev, GetApplicationList()).WillOnce(Return(intList));

  EXPECT_CALL(*serverMock, IsInitialised()).WillOnce(Return(true));
  EXPECT_CALL(*serverMock, CreateConnection(uniq_id, app_handle))
      .WillOnce(Return(TransportAdapter::OK));
  TransportAdapter::Error res = transport_adapter.ConnectDevice(uniq_id);
  EXPECT_EQ(TransportAdapter::OK, res);

  // Try to connect device second time

  EXPECT_CALL(*mockdev, GetApplicationList()).WillOnce(Return(intList));

  EXPECT_CALL(*serverMock, IsInitialised()).WillOnce(Return(true));
  EXPECT_CALL(*serverMock, CreateConnection(uniq_id, app_handle)).Times(0);
  TransportAdapter::Error newres = transport_adapter.ConnectDevice(uniq_id);
  EXPECT_EQ(TransportAdapter::OK, newres);

  EXPECT_CALL(*serverMock, Terminate());
}

TEST(TransportAdapterTest, Disconnect_ConnectDoneSuccess) {
  ServerConnectionFactoryMock* serverMock = new ServerConnectionFactoryMock();
  TestTransportAdapter transport_adapter(NULL, serverMock, NULL);

  EXPECT_CALL(*serverMock, Init()).WillOnce(Return(TransportAdapter::OK));
  transport_adapter.Init();

  int app_handle = 1;
  std::string dev_id = "device_id";

  EXPECT_CALL(*serverMock, IsInitialised()).WillOnce(Return(true));
  EXPECT_CALL(*serverMock, CreateConnection(dev_id, app_handle))
      .WillOnce(Return(TransportAdapter::OK));
  TransportAdapter::Error res = transport_adapter.Connect(dev_id, app_handle);
  EXPECT_EQ(TransportAdapter::OK, res);

  ConnectionMock* mock_connection = new ConnectionMock();
  transport_adapter.ConnectionCreated(mock_connection, dev_id, app_handle);

  transport_adapter.ConnectDone(dev_id, app_handle);

  EXPECT_CALL(*mock_connection, Disconnect())
      .WillOnce(Return(TransportAdapter::OK));
  TransportAdapter::Error new_res =
      transport_adapter.Disconnect(dev_id, app_handle);
  EXPECT_EQ(TransportAdapter::OK, new_res);

  EXPECT_CALL(*serverMock, Terminate());
}

TEST(TransportAdapterTest, DisconnectDevice_DeviceAddedConnectionCreated) {
  ServerConnectionFactoryMock* serverMock = new ServerConnectionFactoryMock();
  TestTransportAdapter transport_adapter(NULL, serverMock, NULL);

  EXPECT_CALL(*serverMock, Init()).WillOnce(Return(TransportAdapter::OK));
  transport_adapter.Init();

  std::string dev_id = "device_id";
  std::string uniq_id = "unique_device_id";
  ;

  DeviceMock* mockdev = new DeviceMock(dev_id, uniq_id);
  transport_adapter.AddDevice(mockdev);

  std::vector<std::string> devList = transport_adapter.GetDeviceList();
  EXPECT_EQ(1, devList.size());
  if (devList.size() == 1) {
    EXPECT_EQ(uniq_id, devList[0]);
  }

  int app_handle = 1;
  std::vector<int> intList = {app_handle};
  EXPECT_CALL(*mockdev, GetApplicationList()).WillOnce(Return(intList));

  EXPECT_CALL(*serverMock, IsInitialised()).WillOnce(Return(true));
  EXPECT_CALL(*serverMock, CreateConnection(uniq_id, app_handle))
      .WillOnce(Return(TransportAdapter::OK));
  TransportAdapter::Error res = transport_adapter.ConnectDevice(uniq_id);
  EXPECT_EQ(TransportAdapter::OK, res);

  ConnectionMock* mock_connection = new ConnectionMock();
  transport_adapter.ConnectionCreated(mock_connection, uniq_id, app_handle);

  EXPECT_CALL(*mock_connection, Disconnect())
      .WillOnce(Return(TransportAdapter::OK));

  TransportAdapter::Error new_res = transport_adapter.DisconnectDevice(uniq_id);
  EXPECT_EQ(TransportAdapter::OK, new_res);

  EXPECT_CALL(*serverMock, Terminate());
}

TEST(TransportAdapterTest, DeviceDisconnected) {
  ServerConnectionFactoryMock* serverMock = new ServerConnectionFactoryMock();
  TestTransportAdapter transport_adapter(NULL, serverMock, NULL);

  EXPECT_CALL(*serverMock, Init()).WillOnce(Return(TransportAdapter::OK));
  transport_adapter.Init();

  std::string dev_id = "device_id";
  std::string uniq_id = "unique_device_id";

  TransportAdapterListenerMock mock_listener;
  transport_adapter.AddListener(&mock_listener);

  DeviceMock* mockdev = new DeviceMock(dev_id, uniq_id);
  EXPECT_CALL(mock_listener, OnDeviceListUpdated(_));
  transport_adapter.AddDevice(mockdev);

  std::vector<std::string> devList = transport_adapter.GetDeviceList();
  EXPECT_EQ(1, devList.size());
  if (devList.size() == 1) {
    EXPECT_EQ(uniq_id, devList[0]);
  }

  int app_handle = 1;
  std::vector<int> intList = {app_handle};
  EXPECT_CALL(*mockdev, GetApplicationList()).WillOnce(Return(intList));

  EXPECT_CALL(*serverMock, IsInitialised()).WillOnce(Return(true));
  EXPECT_CALL(*serverMock, CreateConnection(uniq_id, app_handle))
      .WillOnce(Return(TransportAdapter::OK));
  TransportAdapter::Error res = transport_adapter.ConnectDevice(uniq_id);
  EXPECT_EQ(TransportAdapter::OK, res);

  ConnectionMock* mock_connection = new ConnectionMock();
  transport_adapter.ConnectionCreated(mock_connection, uniq_id, app_handle);

  EXPECT_CALL(*mockdev, GetApplicationList()).WillOnce(Return(intList));
  EXPECT_CALL(mock_listener, OnUnexpectedDisconnect(&transport_adapter, uniq_id,
                                                    app_handle, _));
  EXPECT_CALL(mock_listener,
              OnDisconnectDeviceDone(&transport_adapter, uniq_id));
  EXPECT_CALL(mock_listener, OnDeviceListUpdated(&transport_adapter));
  DisconnectDeviceError error;
  transport_adapter.DeviceDisconnected(uniq_id, error);

  EXPECT_CALL(*serverMock, Terminate());
}

TEST(TransportAdapterTest, AbortedConnectSuccess) {
  ServerConnectionFactoryMock* serverMock = new ServerConnectionFactoryMock();
  TestTransportAdapter transport_adapter(NULL, serverMock, NULL);

  EXPECT_CALL(*serverMock, Init()).WillOnce(Return(TransportAdapter::OK));
  transport_adapter.Init();

  int app_handle = 1;
  std::string dev_id = "device_id";

  EXPECT_CALL(*serverMock, IsInitialised()).WillOnce(Return(true));
  EXPECT_CALL(*serverMock, CreateConnection(dev_id, app_handle))
      .WillOnce(Return(TransportAdapter::OK));
  TransportAdapter::Error res = transport_adapter.Connect(dev_id, app_handle);
  EXPECT_EQ(TransportAdapter::OK, res);

  TransportAdapterListenerMock mock_listener;
  transport_adapter.AddListener(&mock_listener);

  CommunicationError ce;
  EXPECT_CALL(mock_listener, OnUnexpectedDisconnect(_, dev_id, app_handle, _));
  transport_adapter.ConnectionAborted(dev_id, app_handle, ce);

  EXPECT_CALL(*serverMock, Terminate());
}

TEST(TransportAdapterTest, SendData) {
  DeviceScannerMock* dev_mock = new DeviceScannerMock();
  ServerConnectionFactoryMock* serverMock = new ServerConnectionFactoryMock();
  TestTransportAdapter transport_adapter(dev_mock, serverMock, NULL);

  EXPECT_CALL(*dev_mock, Init()).WillOnce(Return(TransportAdapter::OK));
  EXPECT_CALL(*serverMock, Init()).WillOnce(Return(TransportAdapter::OK));
  transport_adapter.Init();

  int app_handle = 1;
  std::string dev_id = "device_id";

  EXPECT_CALL(*serverMock, IsInitialised()).WillOnce(Return(true));
  EXPECT_CALL(*serverMock, CreateConnection(dev_id, app_handle))
      .WillOnce(Return(TransportAdapter::OK));
  TransportAdapter::Error res = transport_adapter.Connect(dev_id, app_handle);
  EXPECT_EQ(TransportAdapter::OK, res);

  ConnectionMock* mock_connection = new ConnectionMock();
  transport_adapter.ConnectionCreated(mock_connection, dev_id, app_handle);

  transport_adapter.ConnectDone(dev_id, app_handle);

  const unsigned int kSize = 3;
  unsigned char data[kSize] = {0x20, 0x07, 0x01};
  const RawMessagePtr kMessage = new RawMessage(1, 1, data, kSize);

  EXPECT_CALL(*mock_connection, SendData(kMessage))
      .WillOnce(Return(TransportAdapter::OK));
  res = transport_adapter.SendData(dev_id, app_handle, kMessage);
  EXPECT_EQ(TransportAdapter::OK, res);

  EXPECT_CALL(*dev_mock, Terminate());
  EXPECT_CALL(*serverMock, Terminate());
}

TEST(TransportAdapterTest, SendData_ConnectionNotEstablished) {
  DeviceScannerMock* dev_mock = new DeviceScannerMock();
  ClientConnectionListenerMock* clientMock = new ClientConnectionListenerMock();
  ServerConnectionFactoryMock* serverMock = new ServerConnectionFactoryMock();
  TestTransportAdapter transport_adapter(dev_mock, serverMock, clientMock);

  EXPECT_CALL(*dev_mock, Init()).WillOnce(Return(TransportAdapter::OK));
  EXPECT_CALL(*clientMock, Init()).WillOnce(Return(TransportAdapter::OK));
  EXPECT_CALL(*serverMock, Init()).WillOnce(Return(TransportAdapter::OK));
  transport_adapter.Init();

  int app_handle = 1;
  std::string dev_id = "device_id";

  EXPECT_CALL(*serverMock, IsInitialised()).WillOnce(Return(true));
  EXPECT_CALL(*serverMock, CreateConnection(dev_id, app_handle))
      .WillOnce(Return(TransportAdapter::OK));
  TransportAdapter::Error res = transport_adapter.Connect(dev_id, app_handle);
  EXPECT_EQ(TransportAdapter::OK, res);

  ConnectionMock* mock_connection = new ConnectionMock();
  transport_adapter.ConnectionCreated(mock_connection, dev_id, app_handle);

  const unsigned int kSize = 3;
  unsigned char data[kSize] = {0x20, 0x07, 0x01};
  const RawMessagePtr kMessage = new RawMessage(1, 1, data, kSize);

  EXPECT_CALL(*mock_connection, SendData(kMessage)).Times(0);
  res = transport_adapter.SendData(dev_id, app_handle, kMessage);
  EXPECT_EQ(TransportAdapter::BAD_PARAM, res);

  EXPECT_CALL(*dev_mock, Terminate());
  EXPECT_CALL(*clientMock, Terminate());
  EXPECT_CALL(*serverMock, Terminate());
}

TEST(TransportAdapterTest, StartClientListening_ClientNotInitialized) {
  DeviceScannerMock* dev_mock = new DeviceScannerMock();
  ClientConnectionListenerMock* clientMock = new ClientConnectionListenerMock();
  TestTransportAdapter transport_adapter(dev_mock, NULL, clientMock);

  EXPECT_CALL(*dev_mock, Init()).WillOnce(Return(TransportAdapter::OK));
  EXPECT_CALL(*clientMock, Init()).WillOnce(Return(TransportAdapter::OK));
  transport_adapter.Init();

  EXPECT_CALL(*clientMock, IsInitialised()).WillOnce(Return(false));
  EXPECT_CALL(*clientMock, StartListening()).Times(0);

  TransportAdapter::Error res = transport_adapter.StartClientListening();
  EXPECT_EQ(TransportAdapter::BAD_STATE, res);

  EXPECT_CALL(*dev_mock, Terminate());
  EXPECT_CALL(*clientMock, Terminate());
}

TEST(TransportAdapterTest, StartClientListening) {
  DeviceScannerMock* dev_mock = new DeviceScannerMock();
  ClientConnectionListenerMock* clientMock = new ClientConnectionListenerMock();
  TestTransportAdapter transport_adapter(dev_mock, NULL, clientMock);

  EXPECT_CALL(*dev_mock, Init()).WillOnce(Return(TransportAdapter::OK));
  EXPECT_CALL(*clientMock, Init()).WillOnce(Return(TransportAdapter::OK));
  transport_adapter.Init();

  EXPECT_CALL(*clientMock, IsInitialised()).WillOnce(Return(true));
  EXPECT_CALL(*clientMock, StartListening())
      .WillOnce(Return(TransportAdapter::OK));

  TransportAdapter::Error res = transport_adapter.StartClientListening();
  EXPECT_EQ(TransportAdapter::OK, res);

  EXPECT_CALL(*dev_mock, Terminate());
  EXPECT_CALL(*clientMock, Terminate());
}

TEST(TransportAdapterTest, StopClientListening_Success) {
  DeviceScannerMock* dev_mock = new DeviceScannerMock();
  ClientConnectionListenerMock* clientMock = new ClientConnectionListenerMock();
  ServerConnectionFactoryMock* serverMock = new ServerConnectionFactoryMock();
  TestTransportAdapter transport_adapter(dev_mock, serverMock, clientMock);

  EXPECT_CALL(*dev_mock, Init()).WillOnce(Return(TransportAdapter::OK));
  EXPECT_CALL(*clientMock, Init()).WillOnce(Return(TransportAdapter::OK));
  EXPECT_CALL(*serverMock, Init()).WillOnce(Return(TransportAdapter::OK));
  transport_adapter.Init();

  int app_handle = 1;
  std::string dev_id = "device_id";

  EXPECT_CALL(*serverMock, IsInitialised()).WillOnce(Return(true));
  EXPECT_CALL(*serverMock, CreateConnection(dev_id, app_handle))
      .WillOnce(Return(TransportAdapter::OK));
  TransportAdapter::Error res = transport_adapter.Connect(dev_id, app_handle);
  EXPECT_EQ(TransportAdapter::OK, res);

  EXPECT_CALL(*clientMock, IsInitialised()).WillOnce(Return(true));
  EXPECT_CALL(*clientMock, StopListening())
      .WillOnce(Return(TransportAdapter::OK));

  res = transport_adapter.StopClientListening();
  EXPECT_EQ(TransportAdapter::OK, res);

  EXPECT_CALL(*dev_mock, Terminate());
  EXPECT_CALL(*clientMock, Terminate());
  EXPECT_CALL(*serverMock, Terminate());
}

TEST(TransportAdapterTest, FindNewApplicationsRequest) {
  DeviceScannerMock* dev_mock = new DeviceScannerMock();
  ClientConnectionListenerMock* clientMock = new ClientConnectionListenerMock();
  ServerConnectionFactoryMock* serverMock = new ServerConnectionFactoryMock();
  TestTransportAdapter transport_adapter(dev_mock, serverMock, clientMock);

  EXPECT_CALL(*dev_mock, Init()).WillOnce(Return(TransportAdapter::OK));
  EXPECT_CALL(*clientMock, Init()).WillOnce(Return(TransportAdapter::OK));
  EXPECT_CALL(*serverMock, Init()).WillOnce(Return(TransportAdapter::OK));
  transport_adapter.Init();

  std::string dev_id = "device_id";
  std::string uniq_id = "unique_device_id";

  TransportAdapterListenerMock mock_listener;
  transport_adapter.AddListener(&mock_listener);

  EXPECT_CALL(mock_listener, OnFindNewApplicationsRequest(&transport_adapter));
  transport_adapter.FindNewApplicationsRequest();

  EXPECT_CALL(*dev_mock, Terminate());
  EXPECT_CALL(*clientMock, Terminate());
  EXPECT_CALL(*serverMock, Terminate());
}

TEST(TransportAdapterTest, GetDeviceAndApplicationLists) {
  TestTransportAdapter transport_adapter(NULL, NULL, NULL);
  transport_adapter.Init();

  std::string dev_id = "device_id";
  std::string uniq_id = "unique_device_id";

  DeviceMock* mockdev = new DeviceMock(dev_id, uniq_id);
  transport_adapter.AddDevice(mockdev);

  std::vector<std::string> devList = transport_adapter.GetDeviceList();
  EXPECT_EQ(1, devList.size());
  if (devList.size() == 1) {
    EXPECT_EQ(uniq_id, devList[0]);
  }

  int app_handle = 1;
  std::vector<int> intList = {app_handle};
  EXPECT_CALL(*mockdev, GetApplicationList()).WillOnce(Return(intList));
  std::vector<int> res = transport_adapter.GetApplicationList(uniq_id);
  EXPECT_EQ(1, res.size());
  if (res.size() == 1) {
    EXPECT_EQ(intList[0], res[0]);
  }
}

TEST(TransportAdapterTest, FindEstablishedConnection) {
  ServerConnectionFactoryMock* serverMock = new ServerConnectionFactoryMock();
  TestTransportAdapter transport_adapter(NULL, serverMock, NULL);

  EXPECT_CALL(*serverMock, Init()).WillOnce(Return(TransportAdapter::OK));
  transport_adapter.Init();

  int app_handle = 1;
  std::string dev_id = "device_id";

  EXPECT_CALL(*serverMock, IsInitialised()).WillOnce(Return(true));
  EXPECT_CALL(*serverMock, CreateConnection(dev_id, app_handle))
      .WillOnce(Return(TransportAdapter::OK));
  TransportAdapter::Error res = transport_adapter.Connect(dev_id, app_handle);
  EXPECT_EQ(TransportAdapter::OK, res);

  ConnectionSPtr mock_connection = new ConnectionMock();
  transport_adapter.ConnectionCreated(mock_connection, dev_id, app_handle);

  transport_adapter.ConnectDone(dev_id, app_handle);

  ConnectionSPtr conn =
      transport_adapter.FindEstabledConnection(dev_id, app_handle);
  EXPECT_EQ(mock_connection, conn);

  EXPECT_CALL(*serverMock, Terminate());
}

}  // namespace transport_manager
}  // namespace components
}  // namespace test
