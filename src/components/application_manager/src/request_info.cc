/*
 * \file request_info.h
 * \brief request information structure source file.
 *
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

#include "application_manager/request_info.h"
namespace application_manager {

namespace request_controller {

CREATE_LOGGERPTR_GLOBAL(logger_, "RequestController");

HMIRequestInfo::HMIRequestInfo(
    RequestPtr request,
    const uint64_t timeout_sec):
  RequestInfo(request, HMIRequest, timeout_sec) {

    correlation_id_ = request_->correlation_id();
    app_id_ = 0;
}

HMIRequestInfo::HMIRequestInfo(
    RequestPtr request,
    const TimevalStruct &start_time,
    const uint64_t timeout_sec):
  RequestInfo(request, HMIRequest, start_time, timeout_sec) {
    correlation_id_ = request_->correlation_id();
    app_id_ = 0;
}

MobileRequestInfo::MobileRequestInfo(
    RequestPtr request,
    const uint64_t timeout_sec):
  RequestInfo(request, MobileRequest, timeout_sec) {
    correlation_id_ = request_.get()->correlation_id();
    app_id_ = request_.get()->connection_key();
}

MobileRequestInfo::MobileRequestInfo(
    RequestPtr request,
    const TimevalStruct &start_time,
    const uint64_t timeout_sec):
  RequestInfo(request, MobileRequest, start_time, timeout_sec) {
    correlation_id_ = request_.get()->correlation_id();
    app_id_ = request_.get()->connection_key();
}

uint64_t RequestInfo::hash() {
  return GenerateHash(app_id(), requestId());
}

uint64_t RequestInfo::GenerateHash(uint32_t var1, uint32_t var2) {
  uint64_t hash_result = 0;
  hash_result = var1;
  hash_result = hash_result << 32;
  hash_result =  hash_result | var2;
  return hash_result;
}

FakeRequestInfo::FakeRequestInfo(uint32_t app_id, uint32_t correaltion_id) {
  app_id_ = app_id;
  correlation_id_ = correaltion_id;
}

bool RequestInfoSet::Add(RequestInfoPtr request_info) {
  std::pair<HashSortedRequestInfoSet::iterator, bool> insert_resilt =
      hash_sorted_pending_requests_.insert(request_info);
  if (insert_resilt.second == true) {
    time_sorted_pending_requests_.insert(request_info);
  } else {
    LOG4CXX_ERROR(logger_, "Request with app_id = " << request_info->app_id()
                  << "; corr_id " << request_info->requestId() << "Already exist ");
  }
  CheckSetSizes();
  return false;
}

RequestInfoPtr RequestInfoSet::Find(const uint32_t connection_key,
                                    const uint32_t correlation_id) {
  RequestInfoPtr result;
  utils::SharedPtr<FakeRequestInfo> request_info_for_search =
      new FakeRequestInfo(connection_key, correlation_id);
  HashSortedRequestInfoSet::iterator it =
      hash_sorted_pending_requests_.find(request_info_for_search);
  if (it != hash_sorted_pending_requests_.end()) {
     result = *it;
  }
  return result;
}

RequestInfoPtr RequestInfoSet::Front() {
  RequestInfoPtr result;
  TimeSortedRequestInfoSet::iterator it = time_sorted_pending_requests_.begin();
  if (it != time_sorted_pending_requests_.end()) {
    result = *it;
  }
  return result;
}


bool RequestInfoSet::Erase(const RequestInfoPtr request_info) {
  size_t erased_count =
      hash_sorted_pending_requests_.erase(request_info);
  bool result = false;
  if (erased_count > 1) {
    LOG4CXX_ERROR(logger_, "Count of erased elements from is not valid : "
                  << erased_count);
    NOTREACHED();
  }
  if (1 == erased_count) {
    time_sorted_pending_requests_.erase(request_info);
    result = true;
  }
  CheckSetSizes();
  return result;
}

void RequestInfoSet::CheckSetSizes() {
  bool set_sizes_equal =
      (time_sorted_pending_requests_.size() == hash_sorted_pending_requests_.size());
  DCHECK(set_sizes_equal);
}

} // namespace request_controller

} // namespace application_manager
