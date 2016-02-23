/*

 Copyright (c) 2013, Ford Motor Company
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:

 Redistributions of source code must retain the above copyright notice, this
 list of conditions and the following disclaimer.

 Redistributions in binary form must reproduce the above copyright notice,
 this list of conditions and the following
 disclaimer in the documentation and/or other materials provided with the
 distribution.

 Neither the name of the Ford Motor Company nor the names of its contributors
 may be used to endorse or promote products derived from this software
 without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef SRC_COMPONENTS_APPLICATION_MANAGER_INCLUDE_APPLICATION_MANAGER_COMMANDS_MOBILE_CREATE_INTERACTION_CHOICE_SET_REQUEST_H_
#define SRC_COMPONENTS_APPLICATION_MANAGER_INCLUDE_APPLICATION_MANAGER_COMMANDS_MOBILE_CREATE_INTERACTION_CHOICE_SET_REQUEST_H_

#include <map>

#include "application_manager/application.h"
#include "application_manager/commands/command_request_impl.h"
#include "application_manager/event_engine/event_observer.h"
#include "interfaces/MOBILE_API.h"
#include "utils/atomic_object.h"

namespace application_manager {

namespace commands {

/**
 * @brief CreateInteractionChoiceSetRequest command class
 **/
class CreateInteractionChoiceSetRequest : public CommandRequestImpl {
 public:
    /**
     * @brief CreateInteractionChoiceSetRequest class constructor
     *
     * @param message Incoming SmartObject message
     **/
    explicit CreateInteractionChoiceSetRequest(const MessageSharedPtr& message);

    /**
     * @brief CreateInteractionChoiceSetRequest class destructor
     **/
    virtual ~CreateInteractionChoiceSetRequest();

    /**
     * @brief Execute command
     **/
    virtual void Run();

 private:
    /**
     * @brief Interface method that is called whenever new event received
     *
     * @param event The received event
     */
    virtual void on_event(const event_engine::Event& event);

    /**
     * @brief Function is called by RequestController when request execution time
     * has exceed it's limit
     */
    virtual void onTimeOut();

    /**
     * @brief Sends VR.AddCommand requests to HMI
     *
     * @param app Application pointer
     *
     */
    void SendVRAddCommandRequests(ApplicationConstSharedPtr app);

    /**
     * @brief Checks incoming choiseSet params.
     *
     * @param app Registered mobile application
     *
     * @return Mobile result code
     */
    mobile_apis::Result::eType CheckChoiceSet(ApplicationConstSharedPtr app);

    /**
     * @brief Checks choice set params(menuName, tertiaryText, ...)
     * When type is String there is a check on the contents \t\n \\t \\n
     * and counts vrCommands array in choice set.
     *
     * @param choice_set which must check
     *
     * @return if choice_set contains \t\n \\t \\n return TRUE, FALSE otherwise
     */
    bool IsWhiteSpaceVRCommandsExist(const smart_objects::SmartObject& choice_set);

    /**
     * @brief DeleteChoices allows to walk through the sent commands map
     * in order to sent appropriate DeleteCommand request.
     */
    void DeleteChoices();

    /**
     * @brief Calls after all responses from HMI were received.
     * Terminates request and sends successful response to mobile
     * if all responses were SUCCESS or calls DeleteChoices in other case.
     */
    void OnAllHMIResponsesReceived();

    /**
     * @brief Id of currently processing choice set
     */
    int32_t choice_set_id_;

    /**
     * @brief The VRCommand struct
     * Collect minimum information about sent VR commands, for correctly
     * processing deleting sent commands if error from HMI received
     */
    struct VRCommandInfo {
      VRCommandInfo() {}
      explicit VRCommandInfo(uint32_t cmd_id):
        cmd_id_(cmd_id),
        succesful_response_received_(false) {}
      uint32_t cmd_id_;
      bool succesful_response_received_;
    };

    /**
     * @brief Map of sent VR.AddCommands
     */
    typedef std::map<uint32_t, VRCommandInfo> SentCommandsMap;
    SentCommandsMap sent_commands_map_;
    sync_primitives::Lock vr_commands_lock_;

    /**
     * @brief Count of VR.AddCommand requests for those
     * we've got response
     */
    size_t received_chs_count_;

    /**
     * @brief Count of VR.AddCommand requests for those
     * we are waiting for response
     */
    sync_primitives::atomic_size_t expected_chs_count_;

    /**
     * @brief Flag shows if one of VR.AddCommand requests was
     * unsuccessful
     */
    sync_primitives::atomic_bool error_from_hmi_;

    /**
     * @brief Flag shows if one of VR.AddCommand requests was
     * WARNINGS
     */
    bool is_warning_from_hmi_;

    /**
     * @brief Number of VR commands in request
     */
    uint32_t amount_vr_commands_;
};

}  // namespace commands

}  // namespace application_manager

#endif  // SRC_COMPONENTS_APPLICATION_MANAGER_INCLUDE_APPLICATION_MANAGER_COMMANDS_MOBILE_CREATE_INTERACTION_CHOICE_SET_REQUEST_H_
