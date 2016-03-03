/*
 * Copyright (c) 2016, Ford Motor Company
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
#include "telemetry_monitor/telemetry_observable.h"
#include "protocol_handler/telemetry_observer.h"
#include "protocol_handler/protocol_handler.h"
#include "protocol_handler/mock_protocol_handler.h"
#include "protocol_handler/mock_session_observer.h"
#include "protocol_handler/mock_protocol_handler_settings.h"
#include "transport_manager/mock_transport_manager.h"
#include "connection_handler/mock_connection_handler.h"

namespace test {
namespace components {
namespace resource_inspector_test {

TEST(ResourceInspectorTest, DISABLED_MessageProcess) {
  // TODO(AK) APPLINK-13351 Disable due to refactor TimeTester
  transport_manager_test::MockTransportManager transport_manager_mock;
  testing::NiceMock<connection_handler_test::MockConnectionHandler>
      connection_handler_mock;
  test::components::protocol_handler_test::MockProtocolHandlerSettings
      protocol_handler_settings_mock;
  test::components::protocol_handler_test::MockSessionObserver
      session_observer_mock;
  protocol_handler::ProtocolHandlerImpl protocol_handler_mock(
      protocol_handler_settings_mock,
      session_observer_mock,
      connection_handler_mock,
      transport_manager_mock);
  resource_inspector::ResourceInspector* resource_inspector =
      new resource_inspector::ResourceInspector();
  // Streamer will be deleted by Thread
  StreamerMock* streamer_mock = new StreamerMock(resource_inspector);
  resource_inspector->set_streamer(streamer_mock);
  resource_inspector->Init(&protocol_handler_mock);
  utils::SharedPtr<resource_inspector::MetricWrapper> test_metric;
  EXPECT_CALL(*streamer_mock, PushMessage(test_metric));
  resource_inspector->SendMetric(test_metric);
  delete resource_inspector;
}

}  // namespace resource_inspector
}  // namespace components
}  // namespace test
