#ifndef STATE_CONTEXT_MOCK_H
#define STATE_CONTEXT_MOCK_H
#include "gmock/gmock.h"
#include "application_manager/state_context.h"

namespace state_controller_test {

class StateContextMock : public application_manager::StateContext {
 public:
  explicit StateContextMock(application_manager::ApplicationManager* app_mngr)
      : StateContext(app_mngr) {}
  MOCK_CONST_METHOD1(is_voice_communication_app, bool(const uint32_t));
};
}

#endif  // STATE_CONTEXT_MOCK_H
