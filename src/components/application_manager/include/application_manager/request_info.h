/*
 * \file request_info.h
 * \brief request information structure header file.
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

#ifndef SRC_COMPONENTS_APPLICATION_MANAGER_INCLUDE_APPLICATION_MANAGER_REQUEST_INFO_H_
#define SRC_COMPONENTS_APPLICATION_MANAGER_INCLUDE_APPLICATION_MANAGER_REQUEST_INFO_H_

#include <stdint.h>
#include <set>

#include "application_manager/commands/command_request_impl.h"
#include "commands/hmi/request_to_hmi.h"

#include "utils/date_time.h"

namespace application_manager {

namespace request_controller {

  /*
   * @brief Typedef for active mobile request
   *
   */
  typedef utils::SharedPtr<commands::Command> RequestPtr;

  struct RequestInfo {
    enum RequestType {MobileRequest, HMIRequest};

    RequestInfo() {}
    virtual ~RequestInfo() {}

    RequestInfo(RequestPtr request,
                const RequestType requst_type,
                const uint64_t timeout_sec)
      : request_(request),
        timeout_sec_(timeout_sec) {
        start_time_ = date_time::DateTime::getCurrentTime();
        updateEndTime();
        requst_type_ = requst_type;
      }

    RequestInfo(RequestPtr request, const RequestType requst_type,
                const TimevalStruct& start_time, const  uint64_t timeout_sec);

    void updateEndTime();

    void updateTimeOut(const uint64_t& timeout_sec);

    bool isExpired();

    TimevalStruct start_time() {
      return start_time_;
    }

    uint64_t timeout_sec() {
      return timeout_sec_;
    }

    TimevalStruct end_time() {
      return end_time_;
    }

    uint32_t app_id() {
      return app_id_;
    }

    mobile_apis::HMILevel::eType hmi_level() {
      return hmi_level_;
    }

    RequestType requst_type() const {
       return requst_type_;
     }

    uint32_t requestId() {
      return correlation_id_;
    }

    commands::Command* request() {
      return request_.get();
    }
    uint64_t hash();
    static uint64_t GenerateHash(uint32_t var1, uint32_t var2);

  protected:
    RequestPtr request_;
    TimevalStruct                 start_time_;
    uint64_t                      timeout_sec_;
    TimevalStruct                 end_time_;
    uint32_t                      app_id_;
    mobile_apis::HMILevel::eType  hmi_level_;
    RequestType                   requst_type_;
    uint32_t                      correlation_id_;
  };

  typedef utils::SharedPtr<RequestInfo> RequestInfoPtr;

  struct MobileRequestInfo: public RequestInfo {
      MobileRequestInfo(RequestPtr request,
                      const uint64_t timeout_sec);
    MobileRequestInfo(RequestPtr request,
                      const TimevalStruct& start_time,
                      const uint64_t timeout_sec);
  };

  struct HMIRequestInfo: public RequestInfo {
    HMIRequestInfo(RequestPtr request, const uint64_t timeout_sec);
    HMIRequestInfo(RequestPtr request, const TimevalStruct& start_time,
                     const  uint64_t timeout_sec);
  };

  struct FakeRequestInfo :public RequestInfo {
      FakeRequestInfo(uint32_t app_id, uint32_t correaltion_id);
  };

  struct RequestInfoTimeComparator {
      bool operator() (const RequestInfoPtr lhs,
                       const RequestInfoPtr rhs) const;
  };

  struct RequestInfoHashComparator {
      bool operator() (const RequestInfoPtr lhs,
                       const RequestInfoPtr rhs) const;
  };

  typedef std::set<RequestInfoPtr, RequestInfoTimeComparator> TimeSortedRequestInfoSet;
  typedef std::set<RequestInfoPtr, RequestInfoHashComparator> HashSortedRequestInfoSet;

  class RequestInfoSet {
    public:
      bool Add(RequestInfoPtr request_info);
      RequestInfoPtr Find(const uint32_t  connection_key,
                          const uint32_t correlation_id);
      RequestInfoPtr Front();
      RequestInfoPtr FrontWithNotNullTimeout();
      bool Erase(const RequestInfoPtr request_info);


      uint32_t RemoveByConnectionKey(uint32_t connection_key);
      uint32_t RemoveMobileRequests();

      const ssize_t Size();

      /**
       * @brief Check if this app is able to add new requests,
       * or limits was exceeded
       * @param app_id - application id
       * @param app_time_scale - time scale (seconds)
       * @param max_request_per_time_scale - maximum count of request
       * that should be allowed for app_time_scale seconds
       * @return True if new request could be added, false otherwise
       */
      bool CheckTimeScaleMaxRequest(uint32_t app_id,
                                    uint32_t app_time_scale,
                                    uint32_t max_request_per_time_scale);

      /**
       * @brief Check if this app is able to add new requests
       * in current hmi_level, or limits was exceeded
       * @param hmi_level - hmi level
       * @param app_id - application id
       * @param app_time_scale - time scale (seconds)
       * @param max_request_per_time_scale - maximum count of request
       * that should be allowed for app_time_scale seconds
       * @return True if new request could be added, false otherwise
       */
      bool CheckHMILevelTimeScaleMaxRequest(mobile_apis::HMILevel::eType hmi_level,
                                            uint32_t app_id,
                                            uint32_t app_time_scale,
                                            uint32_t max_request_per_time_scale);
      TimeSortedRequestInfoSet::iterator GetRequestsByConnectionKey(uint32_t connection_key);

    private:
      struct AppIdCompararator {
          enum CompareType {Equal, NotEqual};
          AppIdCompararator(CompareType compare_type, uint32_t app_id):
            app_id_(app_id),
            compare_type_(compare_type) {}
          bool operator()(const RequestInfoPtr value_compare) const;

        private:
          uint32_t app_id_;
          CompareType compare_type_;
      };

      bool Erase(HashSortedRequestInfoSet::iterator it);
      uint32_t RemoveRequests(const RequestInfoSet::AppIdCompararator& filter);
      inline void CheckSetSizes();
      TimeSortedRequestInfoSet time_sorted_pending_requests_;
      HashSortedRequestInfoSet hash_sorted_pending_requests_;
  };


  /**
  * @brief Structure used in std algorithms to determine amount of request
  * during time scale
  */
  struct TimeScale {
    TimeScale(const TimevalStruct& start,
              const TimevalStruct& end,
              const uint32_t& app_id)
      : start_(start),
        end_(end),
        app_id_(app_id) {}

    bool operator()(RequestInfoPtr setEntry) {
      if (!setEntry.valid()) {
        return false;
      }

      if (setEntry->app_id() != app_id_) {
        return false;
      }

      if (date_time::DateTime::getmSecs(setEntry->start_time())
          < date_time::DateTime::getmSecs(start_) ||
          date_time::DateTime::getmSecs(setEntry->start_time())
          > date_time::DateTime::getmSecs(end_)) {
        return false;
      }

      return true;
    }

  private:
    TimevalStruct  start_;
    TimevalStruct  end_;
    uint32_t       app_id_;
  };

  /**
  * @brief Structure used in std algorithms to determine amount of request
  * during time scale for application in defined hmi level
  */
  struct HMILevelTimeScale {
    HMILevelTimeScale(const TimevalStruct& start,
                      const TimevalStruct& end,
                      const uint32_t& app_id,
                      const mobile_apis::HMILevel::eType& hmi_level)
      : start_(start),
        end_(end),
        app_id_(app_id),
        hmi_level_(hmi_level) {}

    bool operator()(RequestInfoPtr setEntry) {
      if (!setEntry.valid()) {
        return false;
      }

      if (setEntry->app_id() != app_id_) {
        return false;
      }

      if (setEntry->hmi_level() != hmi_level_) {
        return false;
      }

      if (date_time::DateTime::getSecs(setEntry->start_time())
          < date_time::DateTime::getSecs(start_) ||
          date_time::DateTime::getSecs(setEntry->start_time())
          > date_time::DateTime::getSecs(end_)) {
        return false;
      }

      return true;
    }

  private:
    TimevalStruct                start_;
    TimevalStruct                end_;
    uint32_t                     app_id_;
    mobile_apis::HMILevel::eType hmi_level_;
  };

}  //  namespace request_controller

}  //  namespace application_manager
#endif  // SRC_COMPONENTS_APPLICATION_MANAGER_INCLUDE_APPLICATION_MANAGER_REQUEST_INFO_H_
