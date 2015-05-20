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

#include <gtest/gtest.h>
#include <vector>
#include <list>

#include "utils/macro.h"
#include "protocol_handler/protocol_packet.h"

namespace test {
namespace components {
namespace protocol_handler_test {
using namespace ::protocol_handler;

class ProtocolPacketTest : public ::testing::Test {
 protected:
  void SetUp() OVERRIDE {
    some_message_id = 0xABCDEF0;
    some_session_id = 0xFEDCBA0;
    some_connection_id = 10;
  }
  uint32_t some_message_id;
  uint32_t some_session_id;
  ConnectionID some_connection_id;
};

TEST_F(ProtocolPacketTest, SerializePacketWithDiffMalformedVersion) {
  std::vector < uint8_t > malformed_versions;
  for (uint8_t version = PROTOCOL_VERSION_1; version <= PROTOCOL_VERSION_4; ++version) {
    malformed_versions.push_back(version);
  }

  RawMessagePtr res;
  ProtocolPacket prot_packet(some_connection_id, malformed_versions[0], PROTECTION_OFF,
                             FRAME_TYPE_CONTROL, kControl, FRAME_DATA_HEART_BEAT,
                             some_session_id, 0u, some_message_id);
  res = prot_packet.serializePacket();
  EXPECT_EQ(res->protocol_version(), PROTOCOL_VERSION_1);
  EXPECT_EQ(res->service_type(), kControl);
  EXPECT_EQ(res->connection_key(), some_connection_id);
  EXPECT_EQ(res->data_size(), 8u);

  for (size_t i = 1; i < malformed_versions.size(); ++i) {
    ProtocolPacket prot_packet_next(some_connection_id, malformed_versions[i], PROTECTION_OFF,
                                    FRAME_TYPE_CONTROL, kControl, FRAME_DATA_HEART_BEAT,
                                    some_session_id, 0u, some_message_id);
    res = prot_packet_next.serializePacket();
    EXPECT_EQ(res->protocol_version(), malformed_versions[i]);
    EXPECT_EQ(res->service_type(), kControl);
    EXPECT_EQ(res->connection_key(), some_connection_id);
    EXPECT_EQ(res->data_size(), 12u);
  }
}

TEST_F(ProtocolPacketTest, SerializePacketWithWrongMalformedVersion) {
  std::vector < uint8_t > malformed_versions;
  malformed_versions.push_back(0);
  for (uint8_t version = PROTOCOL_VERSION_4 + 1; version <= PROTOCOL_VERSION_MAX; ++version) {
    malformed_versions.push_back(version);
  }
  RawMessagePtr res;
  for (size_t i = 0; i < malformed_versions.size(); ++i) {
    const ProtocolPacket prot_packet(some_connection_id, malformed_versions[i], PROTECTION_OFF,
                                     FRAME_TYPE_CONTROL, kControl, FRAME_DATA_HEART_BEAT,
                                     some_session_id, 0u, some_message_id);
    res = prot_packet.serializePacket();
    EXPECT_EQ(res->protocol_version(), malformed_versions[i]);
    EXPECT_EQ(res->data_size(), 12u);
  }
}

// ServiceType should be equal 0x0 (Control), 0x07 (RPC), 0x0A (PCM), 0x0B (Video), 0x0F (Bulk)
TEST_F(ProtocolPacketTest, SerializePacketWithDiffMalformedServiceType) {
  std::vector < uint8_t > malformed_serv_types;
  malformed_serv_types.push_back(0x0);
  malformed_serv_types.push_back(0x07);
  malformed_serv_types.push_back(0x0A);
  malformed_serv_types.push_back(0x0B);
  malformed_serv_types.push_back(0x0F);

  RawMessagePtr res;
  for (size_t i = 0; i < malformed_serv_types.size(); ++i) {
    ProtocolPacket prot_packet(some_connection_id, PROTOCOL_VERSION_3, PROTECTION_OFF,
                               FRAME_TYPE_CONTROL, malformed_serv_types[i],FRAME_DATA_HEART_BEAT,
                               some_session_id, 0u, some_message_id);
    res = prot_packet.serializePacket();
    EXPECT_EQ(res->protocol_version(), PROTOCOL_VERSION_3);
    EXPECT_EQ(res->service_type(), malformed_serv_types[i]);
    EXPECT_EQ(res->data_size(), 12u);
  }
}

TEST_F(ProtocolPacketTest, SerializePacketWithWrongServiceType) {
  std::vector < uint8_t > malformed_serv_types;
  for (uint8_t service_type = kControl + 1; service_type < kRpc; ++service_type) {
    malformed_serv_types.push_back(service_type);
  }
  malformed_serv_types.push_back(0x08);
  malformed_serv_types.push_back(0x09);
  malformed_serv_types.push_back(0x0C);
  malformed_serv_types.push_back(0x0D);
  malformed_serv_types.push_back(0x0E);

  RawMessagePtr res;
  for (size_t i = 0; i < malformed_serv_types.size(); ++i) {
    ProtocolPacket prot_packet(some_connection_id, PROTOCOL_VERSION_3, PROTECTION_OFF,
                               FRAME_TYPE_CONTROL, malformed_serv_types[i], FRAME_DATA_HEART_BEAT,
                               some_session_id, 0u, some_message_id);
    res = prot_packet.serializePacket();
    EXPECT_EQ(res->protocol_version(), PROTOCOL_VERSION_3);
    EXPECT_EQ(res->service_type(), kInvalidServiceType);
  }
}

TEST_F(ProtocolPacketTest, SetPacketWithWrongFrameType) {
  // Frame type shall be 0x00 (Control), 0x01 (Single), 0x02 (First), 0x03 (Consecutive)
  std::vector < uint8_t > malformed_frame_types;
  for (uint8_t frame_type = FRAME_TYPE_CONSECUTIVE + 1;
       frame_type <= FRAME_TYPE_MAX_VALUE; ++frame_type) {
    malformed_frame_types.push_back(frame_type);
  }

  RawMessagePtr res;
  for (size_t i = 0; i < malformed_frame_types.size(); ++i) {
    ProtocolPacket prot_packet(some_connection_id, PROTOCOL_VERSION_4, PROTECTION_OFF,
                               malformed_frame_types[i], kControl, FRAME_DATA_HEART_BEAT,
                               some_session_id, 0u, some_message_id);
    res = prot_packet.serializePacket();
    EXPECT_EQ(res->protocol_version(), PROTOCOL_VERSION_4);
    EXPECT_EQ(res->service_type(), kControl);
    EXPECT_EQ(prot_packet.frame_type(), malformed_frame_types[i]);
  }
}

TEST_F(ProtocolPacketTest, AppendDataToEmptyPacket) {
  // Set version, serviceType, frameData, sessionId
  uint8_t session_id = 1u;
  uint8_t some_data[] = { 0x0, 0x07, 0x02, session_id };
  ProtocolPacket protocol_packet_;
  RESULT_CODE res = protocol_packet_.appendData(some_data, sizeof(some_data));
  EXPECT_EQ(RESULT_FAIL, res);
}

TEST_F(ProtocolPacketTest, SetTotalDataBytes) {
  uint8_t new_data_size = 10u;
  ProtocolPacket protocol_packet_;
  protocol_packet_.set_total_data_bytes(new_data_size);

  EXPECT_EQ(new_data_size, protocol_packet_.total_data_bytes());
}

TEST_F(ProtocolPacketTest, AppendDataToPacketWithNonZeroSize) {
  // Set version, serviceType, frameData, sessionId
  uint8_t session_id = 1u;
  uint8_t some_data[] = { 0x0, 0x07, FRAME_TYPE_CONTROL, session_id };
  ProtocolPacket protocol_packet_;
  protocol_packet_.set_total_data_bytes(sizeof(some_data) + 1);
  RESULT_CODE res = protocol_packet_.appendData(some_data, sizeof(some_data));
  EXPECT_EQ(RESULT_OK, res);

  EXPECT_EQ(0x0, protocol_packet_.data()[0]);
  EXPECT_EQ(0x07, protocol_packet_.data()[1]);
  EXPECT_EQ(FRAME_TYPE_CONTROL, protocol_packet_.data()[2]);
  EXPECT_EQ(session_id, protocol_packet_.data()[3]);
}

TEST_F(ProtocolPacketTest, SetData) {
  uint8_t session_id = 1u;
  uint8_t some_data[] = { 0x0, 0x07, FRAME_TYPE_CONTROL, session_id };
  ProtocolPacket protocol_packet_;
  protocol_packet_.set_data(some_data, sizeof(some_data));

  EXPECT_EQ(0x0, protocol_packet_.data()[0]);
  EXPECT_EQ(0x07, protocol_packet_.data()[1]);
  EXPECT_EQ(FRAME_TYPE_CONTROL, protocol_packet_.data()[2]);
  EXPECT_EQ(session_id, protocol_packet_.data()[3]);
}

TEST_F(ProtocolPacketTest, DeserializeZeroPacket) {
  uint8_t message[] = { };
  ProtocolPacket protocol_packet_;
  RESULT_CODE res = protocol_packet_.deserializePacket(message, 0);
  EXPECT_EQ(res, RESULT_OK);
}

TEST_F(ProtocolPacketTest, DeserializeNonZeroPacket) {
  // Set header, serviceType, frameData, sessionId
  uint8_t session_id = 1u;
  uint8_t some_message[] = { 0x21, 0x07, 0x02, session_id };
  ProtocolPacket protocol_packet_;
  RESULT_CODE res = protocol_packet_.deserializePacket(some_message,
                                                       PROTOCOL_HEADER_V2_SIZE);
  EXPECT_EQ(res, RESULT_OK);
}

}  // namespace protocol_handler_test
}  // namespace components
}  // namespace test
