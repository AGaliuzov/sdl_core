#ifndef SRC_COMPONENTS_INCLUDE_UTILS_THREADS_THREAD_WATCHER_H_
#define SRC_COMPONENTS_INCLUDE_UTILS_THREADS_THREAD_WATCHER_H_

#include <set>

#include "utils/timer.h"
#include "utils/timer_task_impl.h"
#include "utils/singleton.h"

namespace threads {

class ThreadWatcher: public utils::Singleton<ThreadWatcher> {

 public:
  /**
   * Creates the class instance.
   */
  ThreadWatcher();

  /**
   * @breif Start timer for threads watching.
   * Calles internal OnTimeout function after timer exceeds.
   */
  void StartWatchTimer(uint32_t timeout);

  /**
   * @breif Stops the threads watching.
   */
  void StopWatchTimer();
  
  /**
   * @breif Add certain thread to the watch list.
   */
  void WatchThread(uint32_t tid);

  /**
   * @breif Remove certain thread from the watch list.
   */
  void StopWatching(uint32_t tid);

 private:
  /**
   * Function called after watch timeout happened.
   */
  void OnTimeout();

  timer::Timer timer_;

  bool reduce_priority_;
  std::set<uint32_t> threads_;
  mutable sync_primitives::Lock threads_lock_;
};

} // namespace threads

#endif // SRC_COMPONENTS_INCLUDE_UTILS_THREADS_THREAD_WATCHER_H_
