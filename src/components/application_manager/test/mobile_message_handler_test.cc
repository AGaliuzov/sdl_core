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

#include "application_manager/mobile_message_handler.h"

#include <string>
#include <ctime>

#include "application_manager/message.h"
#include "protocol/raw_message.h"
#include "protocol/message_priority.h"
#include "json/reader.h"
#include "json/writer.h"
#include "json/value.h"
#include "utils/make_shared.h"

#include "gmock/gmock.h"

namespace application_manager {
namespace test {

using protocol_handler::RawMessage;
using protocol_handler::RawMessagePtr;
using protocol_handler::ServiceType;
using protocol_handler::MessagePriority;
using protocol_handler::PROTOCOL_HEADER_V2_SIZE;
using application_manager::MobileMessageHandler;
using ::testing::_;

namespace {

const unsigned char json_size = 0x5e;
unsigned char binary_header[PROTOCOL_HEADER_V2_SIZE] = {
    0x20, 0x00, 0x00, 0xf7, 0x00, 0x00,
    0x00, 0x5c, 0x00, 0x00, 0x00, json_size};

unsigned char data[] =
    "{\n   \"audioStreamingState\" : \"AUDIBLE\",\n   \"hmiLevel\" : "
    "\"FULL\",\n   \"systemContext\" : \"MAIN\"\n}\n";

}  // namespace

class MobileMessageHandlerTest : public testing::Test {
 public:
  MobileMessageHandlerTest() : connection_key_(1) {}

 protected:
  RawMessagePtr message_ptr_;
  const uint32_t connection_key_;

  Message* HandleIncomingMessage(uint32_t protocol_version,
                                 unsigned char* data,
                                 uint32_t payload_size) {
    unsigned int data_size = size(data);
    unsigned char* full_data =
        new unsigned char[PROTOCOL_HEADER_V2_SIZE + data_size];
    uint32_t full_size = PROTOCOL_HEADER_V2_SIZE + data_size;
    memcpy(full_data, binary_header, PROTOCOL_HEADER_V2_SIZE);
    memcpy(full_data + PROTOCOL_HEADER_V2_SIZE, data, data_size);

    message_ptr_ = new RawMessage(connection_key_, protocol_version, full_data,
                                  full_size, ServiceType::kRpc, payload_size);

    delete [] full_data;
    return MobileMessageHandler::HandleIncomingMessageProtocol(message_ptr_);
  }

  size_t size(unsigned char* data) {
    return strlen(reinterpret_cast<char*>(data));
  }

  void TestHandlingIncomingMessageWithBinaryDataProtocol(
      uint32_t protocol_version) {
    // Arrange
    // Add binary data to json message
    unsigned char binary_data[] = "\a\a\a\a";
    uint32_t data_size = size(data);
    uint32_t binary_data_size = size(binary_data);

    unsigned char* json_plus_binary_data =
        new unsigned char[data_size + binary_data_size + 1];

    memcpy(json_plus_binary_data, data, data_size);
    memcpy(json_plus_binary_data + data_size, binary_data,
           binary_data_size + 1);

    uint32_t full_data_size =
        size(data) + PROTOCOL_HEADER_V2_SIZE + binary_data_size;
    // Act
    uint32_t payload_size = size(data);
    Message* message = HandleIncomingMessage(
        protocol_version, json_plus_binary_data, payload_size);

    // Checks
    std::string check_value(data, data + json_size);
    EXPECT_EQ(check_value, message->json_message());
    EXPECT_EQ(1u, message->connection_key());
    EXPECT_EQ(247u, message->function_id());
    EXPECT_EQ(protocol_version, message->protocol_version());
    EXPECT_EQ(0x5c, message->correlation_id());
    EXPECT_EQ(full_data_size, message->data_size());
    EXPECT_EQ(payload_size, message->payload_size());
    EXPECT_TRUE(message->has_binary_data());
    EXPECT_EQ(2, message->type());
    delete[] json_plus_binary_data;
  }

  void TestHandlingIncomingMessageWithoutBinaryDataProtocol(
      uint32_t protocol_version) {
    // Arrange
    uint32_t data_size = size(data);
    uint32_t payload_size = data_size;
    uint32_t full_data_size = data_size + PROTOCOL_HEADER_V2_SIZE;
    Message* message =
        HandleIncomingMessage(protocol_version, data, payload_size);

    // Checks
    std::string check_value(data, data + data_size);
    EXPECT_EQ(check_value, message->json_message());
    EXPECT_EQ(1u, message->connection_key());
    EXPECT_EQ(247u, message->function_id());
    EXPECT_EQ(protocol_version, message->protocol_version());
    EXPECT_EQ(0x5c, message->correlation_id());
    EXPECT_EQ(full_data_size, message->data_size());
    EXPECT_EQ(payload_size, message->payload_size());
    EXPECT_FALSE(message->has_binary_data());
    EXPECT_EQ(2, message->type());
  }

  MobileMessage CreateMessageForSending(uint32_t protocol_version,
                                        uint32_t function_id,
                                        uint32_t correlation_id,
                                        uint32_t connection_key,
                                        const std::string& json_msg,
                                        BinaryData* data = NULL) {
    MobileMessage message = utils::MakeShared<Message>(
        MessagePriority::FromServiceType(ServiceType::kRpc));
    message->set_function_id(function_id);
    message->set_correlation_id(correlation_id);
    message->set_connection_key(connection_key);
    message->set_protocol_version(
        static_cast<ProtocolVersion>(protocol_version));
    message->set_message_type(MessageType::kNotification);
    if (data) {
      message->set_binary_data(data);
    }
    if (!json_msg.empty()) {
      message->set_json_message(json_msg);
    }
    return message;
  }

  void TestHandlingOutgoingMessageProtocolWithoutBinaryData(
      const uint32_t protocol_version) {
    // Arrange
    std::string json_msg(data, data + size(data));
    const uint32_t function_id = 247u;
    const uint32_t correlation_id = 92u;
    const uint32_t connection_key = 1u;

    MobileMessage message_to_send =
        CreateMessageForSending(protocol_version, function_id, correlation_id,
                                connection_key, json_msg);
    // Act
    RawMessage* result_message =
        MobileMessageHandler::HandleOutgoingMessageProtocol(message_to_send);
    uint32_t data_size = PROTOCOL_HEADER_V2_SIZE + json_msg.length();
    uint8_t* data = new uint8_t[data_size];
    memcpy(data, binary_header, PROTOCOL_HEADER_V2_SIZE);
    // Convert json message from string to binary
    std::vector<uint8_t> json_data(json_msg.begin(), json_msg.end());
    uint8_t* json_msg_in_binary = &json_data[0];
    // Create full message
    memcpy(data + PROTOCOL_HEADER_V2_SIZE, json_msg_in_binary,
           json_data.size());

    // Checks
    EXPECT_EQ(protocol_version, result_message->protocol_version());
    EXPECT_EQ(connection_key, result_message->connection_key());
    EXPECT_EQ(data_size, result_message->data_size());
    for (uint8_t i = 0; i < data_size; ++i) {
      EXPECT_EQ(data[i], result_message->data()[i]);
    }
    EXPECT_EQ(ServiceType::kRpc, result_message->service_type());
  }

  void TestHandlingOutgoingMessageProtocolWithBinaryData(
      const uint32_t protocol_version) {
    // Arrange
    std::string json_msg(data, data + size(data));
    BinaryData* bin_dat = new BinaryData;
    bin_dat->push_back('\a');

    const uint32_t function_id = 247u;
    const uint32_t correlation_id = 92u;
    const uint32_t connection_key = 1u;

    MobileMessage message_to_send =
        CreateMessageForSending(protocol_version, function_id, correlation_id,
                                connection_key, json_msg, bin_dat);
    // Act
    RawMessage* result_message =
        MobileMessageHandler::HandleOutgoingMessageProtocol(message_to_send);
    uint32_t data_size =
        PROTOCOL_HEADER_V2_SIZE + json_msg.length() + bin_dat->size();
    uint8_t* data = new uint8_t[data_size];
    memcpy(data, binary_header, PROTOCOL_HEADER_V2_SIZE);

    std::vector<uint8_t> json_data(json_msg.begin(), json_msg.end());
    uint8_t* json_msg_in_binary = &json_data[0];
    memcpy(data + PROTOCOL_HEADER_V2_SIZE, json_msg_in_binary,
           json_data.size());

    // Checks
    EXPECT_EQ(protocol_version, result_message->protocol_version());
    EXPECT_EQ(connection_key, result_message->connection_key());
    EXPECT_EQ(data_size, result_message->data_size());
    for (uint8_t i = 0; i < data_size; ++i) {
      EXPECT_EQ(data[i], result_message->data()[i]);
    }
    EXPECT_EQ(ServiceType::kRpc, result_message->service_type());
  }
};

TEST(mobile_message_test, basic_test) {
  // Example message
  MobileMessage message =
      utils::MakeShared<Message>(protocol_handler::MessagePriority::kDefault);
  EXPECT_FALSE(message->has_binary_data());
  BinaryData* binary_data = new BinaryData;
  binary_data->push_back('X');
  message->set_binary_data(binary_data);
  EXPECT_TRUE(message->has_binary_data());
}

TEST_F(
    MobileMessageHandlerTest,
    Test_HandleIncomingMessageProtocol_MessageWithUnknownProtocolVersion_ExpectNull) {
  // Arrange
  uint32_t data_size = size(data);
  uint32_t payload_size = data_size;
  std::srand(time(0));
  // Generate unknown random protocol version except 1-3
  uint32_t protocol_version = 4 + rand() % UINT32_MAX;
  Message* message =
      HandleIncomingMessage(protocol_version, data, payload_size);

  // Checks
  EXPECT_EQ(NULL, message);
}

TEST_F(
    MobileMessageHandlerTest,
    Test_HandleOutgoingMessageProtocol_MessageWithUnknownProtocolVersion_ExpectNull) {
  // Arrange
  uint32_t data_size = size(data);
  std::srand(time(0));

  const uint32_t function_id = 247u;
  const uint32_t correlation_id = 92u;
  const uint32_t connection_key = 1u;
  // Generate unknown random protocol version except 1-3
  uint32_t protocol_version = 4 + rand() % UINT32_MAX;
  std::string json_msg(data, data + data_size);

  MobileMessage message_to_send = CreateMessageForSending(
      protocol_version, function_id, correlation_id, connection_key, json_msg);
  // Act
  RawMessage* result_message =
      MobileMessageHandler::HandleOutgoingMessageProtocol(message_to_send);

  // Check
  EXPECT_EQ(NULL, result_message);
}

TEST_F(
    MobileMessageHandlerTest,
    Test_HandleIncomingMessageProtocol_MessageWithProtocolV2_WithoutBinaryData) {
  const uint32_t protocol_version = 2u;
  TestHandlingIncomingMessageWithoutBinaryDataProtocol(protocol_version);
}

TEST_F(
    MobileMessageHandlerTest,
    Test_HandleIncomingMessageProtocol_MessageWithProtocolV3_WithoutBinaryData) {
  const uint32_t protocol_version = 3u;
  TestHandlingIncomingMessageWithoutBinaryDataProtocol(protocol_version);
}

TEST_F(
    MobileMessageHandlerTest,
    Test_HandleIncomingMessageProtocol_MessageWithProtocolV2_WithBinaryData) {
  const uint32_t protocol_version = 2u;
  TestHandlingIncomingMessageWithBinaryDataProtocol(protocol_version);
}

TEST_F(
    MobileMessageHandlerTest,
    Test_HandleIncomingMessageProtocol_MessageWithProtocolV3_WithBinaryData) {
  const uint32_t protocol_version = 3u;
  TestHandlingIncomingMessageWithBinaryDataProtocol(protocol_version);
}

TEST_F(
    MobileMessageHandlerTest,
    Test_HandleOutgoingMessageProtocol_MessageWithProtocolV2_WithoutBinaryData) {
  const uint32_t protocol_version = 2u;
  TestHandlingOutgoingMessageProtocolWithoutBinaryData(protocol_version);
}

TEST_F(
    MobileMessageHandlerTest,
    Test_HandleOutgoingMessageProtocol_MessageWithProtocolV3_WithoutBinaryData) {
  const uint32_t protocol_version = 3u;
  TestHandlingOutgoingMessageProtocolWithoutBinaryData(protocol_version);
}

TEST_F(
    MobileMessageHandlerTest,
    Test_HandleOutgoingMessageProtocol_MessageWithProtocolV2_WithBinaryData) {
  const uint32_t protocol_version = 2u;
  TestHandlingOutgoingMessageProtocolWithBinaryData(protocol_version);
}

TEST_F(
    MobileMessageHandlerTest,
    Test_HandleOutgoingMessageProtocol_MessageWithProtocolV3_WithBinaryData) {
  const uint32_t protocol_version = 3u;
  TestHandlingOutgoingMessageProtocolWithBinaryData(protocol_version);
}

}  // namespace test
}  // namespace application_manager
