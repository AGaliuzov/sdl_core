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
#include "protocol_handler/incoming_data_handler.h"
#include "utils/logger.h"
#include "protocol/common.h"

namespace protocol_handler {

CREATE_LOGGERPTR_GLOBAL(logger_, "ProtocolHandler")

IncomingDataHandler::IncomingDataHandler()
  : connections_data_() {
}
static const size_t MIN_HEADER_SIZE = PROTOCOL_HEADER_V1_SIZE;

std::list<ProtocolFramePtr> IncomingDataHandler::ProcessData(const RawMessage& tm_message,
                                                             RESULT_CODE* result) {
  LOG4CXX_TRACE_ENTER(logger_);
  const transport_manager::ConnectionUID connection_id = tm_message.connection_key();
  const uint8_t* data = tm_message.data();
  const size_t tm_message_size = tm_message.data_size();
  if (tm_message_size == 0 || data == NULL) {
    LOG4CXX_WARN(logger_, "Wrong raw message " << tm_message_size << " bytes");
    if (result) {
      *result = RESULT_FAIL;
    }
    LOG4CXX_TRACE_EXIT(logger_);
    return std::list<ProtocolFramePtr>();
  }
  LOG4CXX_DEBUG(logger_, "Processing incoming data of size "
                << tm_message_size << " for connection " << connection_id);
  ConnectionsDataMap::iterator it = connections_data_.find(connection_id);
  if (connections_data_.end() == it) {
    LOG4CXX_ERROR(logger_, "ProcessData requested for unknown connection");
    if (result) {
      *result = RESULT_FAIL;
    }
    LOG4CXX_TRACE_EXIT(logger_);
    return std::list<ProtocolFramePtr>();
  }
  std::vector<uint8_t>& connection_data = it->second;
  connection_data.insert(connection_data.end(), data, data + tm_message_size);
  LOG4CXX_DEBUG(logger_, "Total data size for connection "
                << connection_id << " is " << connection_data.size());
  std::list<ProtocolFramePtr> out_frames;
  while (connection_data.size() >= MIN_HEADER_SIZE) {
    const RESULT_CODE frame_creation_result =
        CreateFrame(connection_data, out_frames, connection_id);

    if (RESULT_DEFRERRED == frame_creation_result) {
      LOG4CXX_DEBUG(logger_, "Wait next portion of data");
      break;
    }
    if (RESULT_OK != frame_creation_result) {
      LOG4CXX_WARN(logger_, "Packet could not be parsed from data stream");
      // TODO(EZamakhov): add to malformed messages counter
      connection_data.clear();
      if (result) {
        *result = frame_creation_result;
      }
      return out_frames;
    }
    LOG4CXX_DEBUG(logger_,
                  "Packet created and passed, new data size for connection "
                  << connection_id << " is " << connection_data.size());
  }
  LOG4CXX_TRACE_EXIT(logger_);

  if (result) {
    *result = RESULT_OK;
  }
  return out_frames;
}

void IncomingDataHandler::AddConnection(
  const transport_manager::ConnectionUID connection_id) {
  // Add empty list of session to new connection
  connections_data_[connection_id] = ConnectionsDataMap::mapped_type();
}

void IncomingDataHandler::RemoveConnection(
  const transport_manager::ConnectionUID connection_id) {
  connections_data_.erase(connection_id);
}

uint32_t IncomingDataHandler::GetPacketSize(
    const ProtocolPacket::ProtocolHeader& header) {
  switch (header.version) {
    case PROTOCOL_VERSION_1:
      return header.dataSize + PROTOCOL_HEADER_V1_SIZE;
    case PROTOCOL_VERSION_2:
    case PROTOCOL_VERSION_3:
      return header.dataSize + PROTOCOL_HEADER_V2_SIZE;
    default:
      LOG4CXX_ERROR(logger_, "Unknown version");
  }
  return 0u;
}

RESULT_CODE IncomingDataHandler::CreateFrame(std::vector<uint8_t>& incomming_data,
                                             std::list<ProtocolFramePtr>& out_frames,
                                             const transport_manager::ConnectionUID connection_id) {
  LOG4CXX_TRACE_ENTER(logger_);
  if (incomming_data.size() >= MIN_HEADER_SIZE) {
    const RESULT_CODE deserialize_header_result =
        validation_header.deserialize(incomming_data.data(),
                                      incomming_data.size());
    if (deserialize_header_result != RESULT_OK) {
      LOG4CXX_TRACE_EXIT(logger_);
      return deserialize_header_result;
    }
    const RESULT_CODE validate_result = validation_header.validate();
    if (validate_result != RESULT_OK) {
      LOG4CXX_TRACE_EXIT(logger_);
      return validate_result;
    }
    LOG4CXX_DEBUG(logger_, "Packet size " << validation_header.dataSize);
    const uint32_t packet_size = GetPacketSize(validation_header);
    if (incomming_data.size() < packet_size) {
      LOG4CXX_DEBUG(logger_, "Packet data is not available yet");
      return RESULT_DEFRERRED;
    }
    ProtocolFramePtr frame(new protocol_handler::ProtocolPacket(connection_id));
    const RESULT_CODE deserialize_result =
        frame->deserializePacket(&incomming_data[0], packet_size);
    if (deserialize_result != RESULT_OK) {
      LOG4CXX_TRACE_EXIT(logger_);
      return deserialize_result;
    }
    out_frames.push_back(frame);
    incomming_data.erase(incomming_data.begin(), incomming_data.begin() + packet_size);
  }
  return RESULT_OK;
}
}  // namespace protocol_handler
