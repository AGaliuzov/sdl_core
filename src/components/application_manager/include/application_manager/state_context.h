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

#ifndef SRC_COMPONENTS_APPLICATION_MANAGER_INCLUDE_APPLICATION_MANAGER_STATE_CONTEXT_H_
#define SRC_COMPONENTS_APPLICATION_MANAGER_INCLUDE_APPLICATION_MANAGER_STATE_CONTEXT_H_

#include <inttypes.h>
#include "application_manager/application_manager.h"
#include "utils/data_accessor.h"

namespace application_manager {
/**
* @brief The StateContext implements access to data, which is required by
* HmiState
*/
class StateContext {
 public:
  explicit StateContext(ApplicationManager* app_mngr);
  virtual ~StateContext();

  /**
   * Executes unary function for each application
   */
  template <typename UnaryFunction>
  void ForEachApplication(UnaryFunction func) const {
    DataAccessor<ApplicationSet> accessor = app_mngr_->applications();
    ApplicationSet::iterator it = accessor.GetData().begin();
    for (; it != accessor.GetData().end(); ++it) {
      ApplicationConstSharedPtr const_app = *it;
      if (const_app) {
        func(app_mngr_->application(const_app->app_id()));
      }
    }
  }

  /**
   * @brief is_navi_app check if app is navi
   * @param app_id application id
   * @return true if app is navi, otherwise return false
   */
  virtual bool is_navi_app(const uint32_t app_id) const;

  /**
   * @brief is_media_app check if app is media
   * @param app_id application id
   * @return true if media_app, otherwise return false
   */
  virtual bool is_media_app(const uint32_t app_id) const;

  /**
   * @brief is_voice_communicationn_app check if app is voice comunication
   * @param app_id application id
   * @return true if voice_communicationn_app, otherwise return false
   */
  virtual bool is_voice_communication_app(const uint32_t app_id) const;

  /**
   * @brief is_attenuated_supported check if HMI support attenuated mode
   * @return true if attenuated supported, otherwise return false
   */
  virtual bool is_attenuated_supported() const;

  /**
   * @brief setApp_mngr setter for application_manager mamber
   * @param app_mngr ApplicationManager instance
   */
  virtual void set_app_mngr(ApplicationManager* app_mngr);

  /**
   * @brief OnHMILevelChanged is proxy for aplication manager OnHMILevelChanged
   * @param app_id id of application, whose hmi level was changed
   * @param from old hmi_level
   * @param to new hmi level
   */
  virtual void OnHMILevelChanged(uint32_t app_id, mobile_apis::HMILevel::eType from,
                         mobile_apis::HMILevel::eType to) const;

  /**
   * @brief Sends HMI status notification to mobile
   *
   *@param application_impl application with changed HMI status
   *
   **/
  virtual void SendHMIStatusNotification(const utils::SharedPtr<Application> app) const;

  /**
   * @brief application_id return application id of request with correlation id
   * @param correlation_id correlation id of request
   * @return application id
   */
  virtual const uint32_t application_id(const int32_t correlation_id) const;

  /**
   * @brief application_by_hmi_app application by hmi applicatin id
   * @param hmi_app_id hmi application id
   * @return application if exist or empty SharedPointer
   */
  virtual ApplicationSharedPtr application_by_hmi_app(const uint32_t hmi_app_id) const;

  virtual mobile_api::HMILevel::eType GetDefaultHmiLevel(
      ApplicationConstSharedPtr application) const;

 private:
  ApplicationManager* app_mngr_;
};
}  // namespace application_manager
#endif  // SRC_COMPONENTS_APPLICATION_MANAGER_INCLUDE_APPLICATION_MANAGER_STATE_CONTEXT_H_
