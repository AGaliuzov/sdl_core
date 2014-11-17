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

#include <errno.h>

#include <limits.h>
#include <stddef.h>
#include <signal.h>

#include "utils/atomic.h"
#include "utils/threads/thread.h"
#include "utils/threads/thread_manager.h"
#include "utils/logger.h"
#include "pthread.h"

 #ifdef BUILD_TESTS
  // Temporary fix for UnitTest until APPLINK-9987 is resolved
 #include <unistd.h>
 #endif

#ifndef __QNXNTO__
  const int EOK = 0;
#endif

#if defined(OS_POSIX)
  const size_t THREAD_NAME_SIZE = 15;
#endif

namespace threads {

CREATE_LOGGERPTR_GLOBAL(logger_, "Utils")

namespace {
enum ThreadState {
  THREAD_INIT,
  THREAD_STARTED,
  THREAD_RUNNING,
  THREAD_STOPPED
};

void enqueue_to_join(pthread_t thread, ThreadDelegate* delegate) {
  MessageQueue<ThreadManager::ThreadDesc>& threads = ::threads::ThreadManager::instance()->threads_to_terminate;
  if (!threads.IsShuttingDown() ) {
    LOG4CXX_DEBUG(logger_, "Pushing thread #" << pthread_self() << " to join queue");
    ThreadManager::ThreadDesc desc = { thread, delegate };
    threads.push(desc);
  }
}

static void* threadFunc(void* arg) {
  LOG4CXX_DEBUG(logger_, "Thread #" << pthread_self() << " started successfully");
  threads::ThreadDelegate* delegate = static_cast<threads::ThreadDelegate*>(arg);

  delegate->thread_lock().Acquire();
  threads::Thread* thread = delegate->CurrentThread();
  if (thread) {
    thread->set_running(true);
  }
  delegate->thread_lock().Release();

  delegate->threadMain();

  delegate->thread_lock().Acquire();
  thread = delegate->CurrentThread();
  if (thread) {
    thread->set_running(false);
  }
  delegate->thread_lock().Release();

  enqueue_to_join(pthread_self(), delegate);
  LOG4CXX_DEBUG(logger_, "Thread #" << pthread_self() << " exited successfully");
  return NULL;
}
}  // namespace

size_t Thread::kMinStackSize = PTHREAD_STACK_MIN; /* Ubuntu : 16384 ; QNX : 256; */

void Thread::SetNameForId(const Id& thread_id, const std::string& name) {
  std::string nm = name;
  std::string& trimname = nm.size() > 15 ? nm.erase(15) : nm;
  const int rc = pthread_setname_np(thread_id.id_, trimname.c_str());
  if(rc != EOK) {
    LOG4CXX_WARN(logger_, "Couldn't set pthread name \""
                       << trimname
                       << "\", error code "
                       << rc
                       << " ("
                       << strerror(rc)
                       << ")");
  }
}

Thread::Thread(const char* name, ThreadDelegate* delegate)
  : name_(name ? name : "undefined"),
    delegate_(delegate),
    thread_handle_(0),
    thread_options_(),
    isThreadRunning_(0) { }

ThreadDelegate* Thread::delegate() const {
  return delegate_;
}

bool Thread::start() {
  return startWithOptions(thread_options_);
}

Thread::Id Thread::CurrentId() {
  return Id(pthread_self());
}

bool Thread::startWithOptions(const ThreadOptions& options) {
  LOG4CXX_TRACE_ENTER(logger_);

  if (!delegate_) {
    LOG4CXX_ERROR(logger_, "NULL delegate");
    LOG4CXX_TRACE_EXIT(logger_);
    return false;
  }

  thread_options_ = options;

  pthread_attr_t attributes;
  int pthread_result = pthread_attr_init(&attributes);
  if (pthread_result != EOK) {
    LOG4CXX_WARN(logger_,"Couldn't init pthread attributes. Error code = "
                 << pthread_result << "(\"" << strerror(pthread_result) << "\")");
  }

  if (!thread_options_.is_joinable()) {
    pthread_result = pthread_attr_setdetachstate(&attributes, PTHREAD_CREATE_DETACHED);
    if (pthread_result != EOK) {
      LOG4CXX_WARN(logger_,"Couldn't set detach state attribute.. Error code = "
                   << pthread_result << "(\"" << strerror(pthread_result) << "\")");
      thread_options_.is_joinable(false);
    }
  }

  const size_t stack_size = thread_options_.stack_size();
  if (stack_size >= Thread::kMinStackSize) {
    pthread_result = pthread_attr_setstacksize(&attributes, stack_size);
    if (pthread_result != EOK) {
      LOG4CXX_WARN(logger_,"Couldn't set stacksize = " << stack_size <<
                   ". Error code = " << pthread_result << "(\""
                   << strerror(pthread_result) << "\")");
    }
  }

  delegate_->run_ = true;
  pthread_result = pthread_create(&thread_handle_, &attributes, threadFunc, delegate_);
  if (pthread_result != EOK) {
    LOG4CXX_WARN(logger_, "Couldn't create thread. Error code = "
                 << pthread_result << "(\"" << strerror(pthread_result) << "\")");
  } else {
    LOG4CXX_INFO(logger_,"Created thread: " << name_);
    SetNameForId(Id(thread_handle_), name_);
  }
  LOG4CXX_TRACE_EXIT(logger_);
  return pthread_result == EOK;
}

void Thread::stop() {
  LOG4CXX_TRACE_ENTER(logger_);

#ifdef BUILD_TESTS
  // Temporary fix for UnitTest until APPLINK-9987 is resolved
  usleep(100000);
#endif

  LOG4CXX_DEBUG(logger_, "Stopping thread #" << thread_handle_
                  << " \""  << name_ << " \"");

  sync_primitives::AutoLock auto_lock(delegate_lock_);
  if (delegate_) {
    delegate_->run_ = false;
  }

  if (!atomic_post_clr(&isThreadRunning_)) {

    LOG4CXX_DEBUG(logger_, "Thread #" << thread_handle_
                  << " \""  << name_ << " \" is not running");
    LOG4CXX_TRACE_EXIT(logger_);
    return;
  }

  // TODO(EZamakhov): fix segfault in case exitThreadMain return false and correctly finished
  if (delegate_ && !delegate_->exitThreadMain()) {
    if (thread_handle_ != pthread_self()) {
      LOG4CXX_WARN(logger_, "Cancelling thread #" << thread_handle_
                   << " \""  << name_ << " \"");
      const int pthread_result = pthread_cancel(thread_handle_);
      if (pthread_result == EOK) {
        enqueue_to_join(thread_handle_, delegate_);
      } else {
        LOG4CXX_ERROR(logger_,
                     "Couldn't cancel thread (#" << thread_handle_ << " \"" << name_ <<
                     "\") from thread #" << pthread_self() << ". Error code = "
                     << pthread_result << " (\"" << strerror(pthread_result) << "\")");
      }
    } else {
      enqueue_to_join(thread_handle_, delegate_);
      LOG4CXX_WARN(logger_, "Exiting from thread #" << thread_handle_);
      pthread_exit(NULL);
    }
  }
  LOG4CXX_DEBUG(logger_, "Stopped thread #" << thread_handle_
                << " \""  << name_ << " \"");
  LOG4CXX_TRACE_EXIT(logger_);
}

void Thread::join() {
  LOG4CXX_TRACE_ENTER(logger_);
  if (isThreadRunning_) {
    pthread_join(thread_handle_, NULL);
  } else {
    LOG4CXX_DEBUG(logger_, "Thread #" << thread_handle_
                  << " \""  << name_ << " \" is not running");
  }
  LOG4CXX_TRACE_EXIT(logger_);
}

bool Thread::Id::operator==(const Thread::Id& other) const {
  return pthread_equal(id_, other.id_) != 0;
}

std::ostream& operator<<(std::ostream& os, const Thread::Id& thread_id) {
  char name[32];
  if(pthread_getname_np(thread_id.Handle(), name, 32) == 0) {
    os << name;
  }
  return os;
}


Thread* CreateThread(const char* name, ThreadDelegate* delegate) {
  delegate->thread_lock().Acquire();
  Thread* thread = new Thread(name, delegate);
  delegate->thread_ = thread;
  delegate->thread_lock().Release();
  return thread;
}

void DeleteThread(Thread* thread) {
  if (thread) {
    thread->delegate_lock_.Acquire();
    ThreadDelegate* delegate = thread->delegate();
    if (delegate) {
      delegate->thread_lock().Acquire();
      delegate->thread_ = NULL;
      delegate->thread_lock().Release();
    }
    thread->delegate_lock_.Release();
    delete thread;
  }
}


}  // namespace threads
