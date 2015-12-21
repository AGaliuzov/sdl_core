
#include "utils/mock_object.h"

namespace test {
namespace components {
namespace utils_test {

CMockObject::CMockObject()
    : mId_(0) {
}

CMockObject::CMockObject(int id)
    : mId_(id) {
}

CMockObject::~CMockObject() {
  destructor();
}

int CMockObject::getId() const {
  return mId_;
}

}  // namespace utils_test
}  // namespace components
}  // namespace test
