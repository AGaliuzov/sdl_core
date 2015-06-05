#ifndef SRC_COMPONENTS_INCLUDE_UTILS_ATOMIC_H_
#define SRC_COMPONENTS_INCLUDE_UTILS_ATOMIC_H_

#include "utils/rwlock.h"
#include "utils/conditional_variable.h"
#include "utils/macro.h"

namespace sync_primitives {

/**
 * @brief Allows to safely change stored value from different threads.
 *
 * The usage example:
 *
 * threads::Atomic<int> i;
 *
 * i = 5; // here SDL is able to guarantee that this value will be safely
 *        // assigned even in multi threaded environment.
 */
template <typename T>
class Atomic {
 public:
  /**
   * @brief Atomic allows to construct atomic object.
   * The operation is not atomic.
   *
   * @param value the value to initialize object with.
   */
  Atomic(const T& value) : value_(value) {}

  /**
   * @brief operator = thread safe setter for stored value.
   *
   * @param val value to assign.
   *
   * @return mofified value.
   */
  T& operator=(const T& val) {
    sync_primitives::AutoWriteLock lock(rw_lock_);
    value_ = val;
  }

  /**
   * @brief operator T thread safe getter
   *
   * return stored value.
   */
  operator T() const {
    sync_primitives::AutoReadLock lock(rw_lock_);
    return value_;
  }

  /**
   * @brief operator T thread safe getter
   *
   * return stored value.
   */
  template <typename U>
  operator U() const {
    sync_primitives::AutoReadLock lock(rw_lock_);
    return static_cast<U>(value_);
  }

 private:
  T value_;
  mutable sync_primitives::RWLock rw_lock_;
};

typedef Atomic<int> atomic_int;
typedef Atomic<int32_t> atomic_int32;
typedef Atomic<uint32_t> atomic_uint32;
typedef Atomic<int64_t> atomic_int64;
typedef Atomic<uint64_t> atomic_uint64;
typedef Atomic<size_t> atomic_size_t;
typedef Atomic<bool> atomic_bool;
}  // namespace sync_primitives
#endif  // SRC_COMPONENTS_INCLUDE_UTILS_ATOMIC_H_
