/**
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

#ifndef SRC_COMPONENTS_APPLICATION_MANAGER_INCLUDE_APPLICATION_MANAGER_REQUEST_CONTROLLER_H_
#define SRC_COMPONENTS_APPLICATION_MANAGER_INCLUDE_APPLICATION_MANAGER_REQUEST_CONTROLLER_H_

#include <climits>
#include <vector>
#include <list>

#include "utils/lock.h"
#include "utils/shared_ptr.h"
#include "utils/threads/thread.h"
#include "utils/conditional_variable.h"
#include "utils/threads/thread_delegate.h"

#include "interfaces/MOBILE_API.h"
#include "interfaces/HMI_API.h"

#include "application_manager/request_info.h"
#include "utils/timer_thread.h"


namespace application_manager {

namespace request_controller {

using namespace threads;

/**
* @brief RequestController class is used to control currently active mobile
* requests.
*/
class RequestController {
  public:

    /**
    * @brief Result code for addRequest
    */
    enum TResult
    {
      SUCCESS = 0,
      TOO_MANY_REQUESTS,
      TOO_MANY_PENDING_REQUESTS,
      NONE_HMI_LEVEL_MANY_REQUESTS,
      INVALID_DATA
    };

    /**
    * @brief Thread pool state
    */
    enum TPoolState
    {
      UNDEFINED = 0,
      STARTED,
      STOPPED,
    };

    // Methods

    /**
    * @brief Class constructor
    *
    */
    RequestController();

    /**
    * @brief Class destructor
    *
    */
    virtual ~RequestController();

    /**
    * @brief Initialize thread pool
    *
    */
    void  InitializeThreadpool();

    /**
    * @brief Destroy thread pool
    *
    */
    void DestroyThreadpool();

    /**
    * @brief Check if max request amount wasn't exceed and adds request to queue.
    *
    * @param request     Active mobile request
    * @param hmi_level   Current application hmi_level
    *
    * @return Result code
    *
    */
    TResult addMobileRequest(const RequestPtr& request,
                             const mobile_apis::HMILevel::eType& hmi_level);


    /**
    * @brief Store HMI request until response or timeout won't remove it
    *
    * @param request     Active hmi request
    * @return Result code
    *
    */
    TResult addHMIRequest(const RequestPtr request);

    /**
    * @ Add notification to collection
    *
    * @param ptr Reference to shared pointer that point on hmi notification
    */
    void addNotification(const RequestPtr ptr);

    /**
    * @brief Removes request from queue
    *
    * @param mobile_corellation_id Active mobile request correlation ID
    *
    */
    void terminateMobileRequest(const uint32_t& mobile_correlation_id);


    /**
    * @brief Removes request from queue
    *
    * @param mobile_corellation_id Active mobile request correlation ID
    *
    */
    void terminateHMIRequest(const uint32_t& correlation_id);

    /**
    * @ Add notification to collection
    *
    * @param ptr Reference to shared pointer that point on hmi notification
    */
    void removeNotification(const commands::Command* notification);

    /**
    * @brief Removes all requests from queue for specified application
    *
    * @param app_id Mobile application ID (app_id)
    *
    */
    void terminateAppRequests(const uint32_t& app_id);

    /**
    * @brief Terminates all requests from HMI
    */
    void terminateAllHMIRequests();


    /**
    * @brief Terminates all requests from Mobile
    */
    void terminateAllMobileRequests();

    /**
    * @brief Updates request timeout
    *
    * @param app_id Connection key of application
    * @param mobile_correlation_id Correlation ID of the mobile request
    * @param new_timeout_value New timeout to be set in milliseconds
    */
    void updateRequestTimeout(const uint32_t& app_id,
                              const uint32_t& mobile_correlation_id,
                              const uint32_t& new_timeout);

    /*
     * @brief Function Should be called when Low Voltage is occured
     */
    void OnLowVoltage();

    /*
     * @brief Function Should be called when Low Voltage is occured
     */
    void OnWakeUp();

    bool IsLowVoltage();
  protected:

    /**
     * @brief Check if this app is able to add new requests, or limits was exceeded
     * @param app_id - application id
     * @param app_time_scale - time scale (seconds)
     * @param max_request_per_time_scale - maximum count of request that should be allowed for app_time_scale seconds
     * @return True if new request could be added, false otherwise
     */
    bool CheckTimeScaleMaxRequest(const uint32_t& app_id,
                                  const uint32_t& app_time_scale,
                                  const uint32_t& max_request_per_time_scale);

    /**
     * @brief Check if this app is able to add new requests in current hmi_level, or limits was exceeded
     * @param hmi_level - hmi level
     * @param app_id - application id
     * @param app_time_scale - time scale (seconds)
     * @param max_request_per_time_scale - maximum count of request that should be allowed for app_time_scale seconds
     * @return True if new request could be added, false otherwise
     */
    bool CheckHMILevelTimeScaleMaxRequest(const mobile_apis::HMILevel::eType& hmi_level,
                                          const uint32_t& app_id,
                                          const uint32_t& app_time_scale,
                                          const uint32_t& max_request_per_time_scale);

    /**
     * @brief Check Posibility to add new requests, or limits was exceeded
     * @param pending_requests_amount - maximum count of request that should be allowed for all applications
     * @return True if new request could be added, false otherwise
     */
    bool CheckPendingRequestsAmount(const uint32_t& pending_requests_amount);

    void onTimer();

    /**
    * @brief Update timout for next OnTimer
    * Not thread safe
    */
    void UpdateTimer();


  private:

    // Data types

    class Worker : public ThreadDelegate {
      public:
        Worker(RequestController* requestController);
        virtual ~Worker();
        virtual void threadMain();
        virtual bool exitThreadMain();
      protected:
      private:
        RequestController*                               request_controller_;
        sync_primitives::Lock                             thread_lock_;
        volatile bool                                    stop_flag_;
    };

    std::vector<Thread*> pool_;
    volatile TPoolState pool_state_;
    uint32_t pool_size_;
    sync_primitives::ConditionalVariable cond_var_;

    std::list<RequestPtr> mobile_request_list_;
    sync_primitives::Lock mobile_request_list_lock_;

    RequestInfoSet pending_request_set_;
    sync_primitives::Lock pending_request_set_lock_;

    /**
    * @brief Set of HMI notifications with timeout.
    */
    std::list<RequestPtr> notification_list_;

    timer::TimerThread<RequestController>  timer_;
    static const uint32_t dafault_sleep_time_ = UINT_MAX;

    bool is_low_voltage_;
    DISALLOW_COPY_AND_ASSIGN(RequestController);
};

}  // namespace request_controller

}  // namespace application_manager

#endif  // SRC_COMPONENTS_APPLICATION_MANAGER_INCLUDE_APPLICATION_MANAGER_REQUEST_CONTROLLER_H_
