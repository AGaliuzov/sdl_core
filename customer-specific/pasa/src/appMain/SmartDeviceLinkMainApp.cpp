#include <fcntl.h>
#include <semaphore.h>
#include <string.h>
#include <log4cxx/rollingfileappender.h>
#include <log4cxx/fileappender.h>
#include <errno.h>

#include "./life_cycle.h"
#include "SmartDeviceLinkMainApp.h"
#include "signal_handlers.h"

#include "utils/macro.h"
#include "utils/logger.h"
#include "utils/system.h"
#include "utils/signals.h"
#include "utils/file_system.h"
#include "utils/log_message_loop_thread.h"
#include "config_profile/profile.h"
#include "utils/appenders_loader.h"
#include "utils/threads/thread.h"
#include "utils/timer_thread.h"

#include "hmi_message_handler/hmi_message_handler_impl.h"
#include "hmi_message_handler/messagebroker_adapter.h"
#include "application_manager/application_manager_impl.h"
#include "connection_handler/connection_handler_impl.h"
#include "protocol_handler/protocol_handler_impl.h"
#include "transport_manager/transport_manager.h"
#include "transport_manager/transport_manager_default.h"
// ----------------------------------------------------------------------------
// Third-Party includes

#include "CMessageBroker.hpp"
#include "mb_tcpserver.hpp"
#include "networking.h"  // cpplint: Include the directory when naming .h files
#include "system.h"      // cpplint: Include the directory when naming .h files
// ----------------------------------------------------------------------------

bool g_bTerminate = false;

namespace {
  const char kShellInterpreter[] = "sh";
  const char kPolicyInitializationScript[] = "/fs/mp/etc/AppLink/init_policy.sh";
}

// --------------------------------------------------------------------------
// Logger initialization

CREATE_LOGGERPTR_GLOBAL(logger_, "SmartDeviceLinkMainApp")

// TODO: Remove this code when PASA will support SDL_MSG_START_USB_LOGGING
// Start section
bool remoteLoggingFlagFileExists(const std::string& name) {
  LOG4CXX_INFO(logger_, "Check path: " << name);

  struct stat buffer;
  return (stat (name.c_str(), &buffer) == 0);
}

bool remoteLoggingFlagFileValid() {
  return true;
}
// End section

int getBootCount() {
  std::string fileContent;
  int bootCount;

  file_system::ReadFile(profile::Profile::instance()->target_boot_count_file(),
                        fileContent);

  std::stringstream(fileContent) >> bootCount;

  return bootCount;
}

void setupRollingFileLogger() {
  log4cxx::helpers::Pool p;
  int bootCount = getBootCount();
  std::string paramFileAppender = "RollingLogFile";
  std::stringstream stream;
  stream << bootCount;
  std::string paramLogFileMaxSize = profile::Profile::instance()->log_file_max_size();
  std::string paramFileName = profile::Profile::instance()->target_log_file_home_dir() +
                           stream.str() + "." +
                           profile::Profile::instance()->target_log_file_name_pattern();

  LOG4CXX_DECODE_CHAR(logFileAppender, paramFileAppender);
  LOG4CXX_DECODE_CHAR(logFileName, paramFileName);
  LOG4CXX_DECODE_CHAR(logFileMaxSize, paramLogFileMaxSize);

  log4cxx::LoggerPtr rootLogger = logger_->getLoggerRepository()->getRootLogger();
  log4cxx::RollingFileAppenderPtr fileAppender = rootLogger->getAppender(logFileAppender);

  rootLogger->removeAppender("LogFile");

  if(fileAppender != NULL) {
    //fileAppender->setAppend(true);
    fileAppender->setFile(logFileName);
    fileAppender->setMaxFileSize(logFileMaxSize);
    fileAppender->activateOptions(p);
  }
}

void setupFileLogger() {
  log4cxx::helpers::Pool p;
  int bootCount = getBootCount();
  std::string paramFileAppender = "LogFile";
  std::stringstream stream;
  stream << bootCount;
  std::string paramFileName = profile::Profile::instance()->target_log_file_home_dir() +
                           stream.str() + "." +
                           profile::Profile::instance()->target_log_file_name_pattern();

  LOG4CXX_DECODE_CHAR(logFileAppender, paramFileAppender);
  LOG4CXX_DECODE_CHAR(logFileName, paramFileName );

  log4cxx::LoggerPtr rootLogger = logger_->getLoggerRepository()->getRootLogger();
  log4cxx::FileAppenderPtr fileAppender = rootLogger->getAppender(logFileAppender);

  rootLogger->removeAppender("RollingLogFile");

  if(fileAppender != NULL) {
    //fileAppender->setAppend(true);
    fileAppender->setFile(logFileName);
    fileAppender->activateOptions(p);
  }
}

void configureLogging() {
  if (0 == strcmp("0K", profile::Profile::instance()->log_file_max_size().c_str())) {
    setupFileLogger();
  } else {
    setupRollingFileLogger();
  }
}

void startUSBLogging() {
  LOG4CXX_INFO(logger_, "Enable logging to USB");

  int bootCount = getBootCount();
  std::stringstream stream;
  stream << bootCount;
  std::string currentLogFilePath = profile::Profile::instance()->target_log_file_home_dir() +
            stream.str() + "." +
            profile::Profile::instance()->target_log_file_name_pattern();
  std::string usbLogFilePath = profile::Profile::instance()->remote_logging_flag_file_path() +
                       stream.str() + "." +
                       profile::Profile::instance()->target_log_file_name_pattern();

  if (!currentLogFilePath.empty()) {
     LOG4CXX_INFO(logger_, "Copy existing log info to USB from " << currentLogFilePath);

     std::vector<uint8_t> targetLogFileContent;

     file_system::ReadBinaryFile(currentLogFilePath, targetLogFileContent);

     file_system::WriteBinaryFile(usbLogFilePath, targetLogFileContent);
  }

  log4cxx::helpers::Pool p;

  std::string paramFileAppender = "LogFile";
  std::string paramFileName = usbLogFilePath;

  LOG4CXX_DECODE_CHAR(logFileAppender, paramFileAppender);
  LOG4CXX_DECODE_CHAR(logFileName, paramFileName );

  log4cxx::LoggerPtr rootLogger = logger_->getLoggerRepository()->getRootLogger();

  log4cxx::FileAppenderPtr fileAppender = rootLogger->getAppender(logFileAppender);

  if(fileAppender != NULL) {
    fileAppender->setAppend(true);
    fileAppender->setFile(logFileName);
    fileAppender->activateOptions(p);
  }
}

void startSmartDeviceLink()
{
  LOG4CXX_INFO(logger_, " Application started!");
  LOG4CXX_INFO(logger_, "Boot count: " << getBootCount());

  // --------------------------------------------------------------------------
  // Components initialization

  // TODO: Remove this code when PASA will support SDL_MSG_START_USB_LOGGING
  // Start section
  if (remoteLoggingFlagFileExists(
          profile::Profile::instance()->remote_logging_flag_file_path() +
          profile::Profile::instance()->remote_logging_flag_file()) &&
          remoteLoggingFlagFileValid()) {

    startUSBLogging();

  }
  // End section

  if (!main_namespace::LifeCycle::instance()->StartComponents()) {
    LOG4CXX_FATAL(logger_, "Failed start components");
#ifdef ENABLE_LOG
    logger::LogMessageLoopThread::destroy();
#endif
    DEINIT_LOGGER();
    exit(EXIT_FAILURE);
  }

  // --------------------------------------------------------------------------
  // Third-Party components initialization.

  if (!main_namespace::LifeCycle::instance()->InitMessageSystem()) {
    LOG4CXX_FATAL(logger_, "Failed InitMessageBroker");
#ifdef ENABLE_LOG
    logger::LogMessageLoopThread::destroy();
#endif
    DEINIT_LOGGER();
    exit(EXIT_FAILURE);
  }
  LOG4CXX_INFO(logger_, "InitMessageBroker successful");

  // --------------------------------------------------------------------------
}

void stopSmartDeviceLink()
{
  LOG4CXX_INFO(logger_, " LifeCycle stopping!");
  main_namespace::LifeCycle::instance()->StopComponents();
  LOG4CXX_INFO(logger_, " LifeCycle stopped!");

  g_bTerminate = true;
}

class ApplinkNotificationThreadDelegate : public threads::ThreadDelegate {
 public:
  ApplinkNotificationThreadDelegate(int fd);
  ~ApplinkNotificationThreadDelegate();
  virtual void threadMain();
 private:
  int readfd_;
  void init_mq(const std::string& name, int flags, mqd_t& mq_desc);
  void close_mq(mqd_t mq_to_close);
  void sendHeartBeat();
  utils::SharedPtr<timer::TimerThread<ApplinkNotificationThreadDelegate> > heart_beat_sender_;
  mqd_t mq_from_sdl_;
  struct mq_attr attributes_;
  size_t heart_beat_timeout_;
};

ApplinkNotificationThreadDelegate::ApplinkNotificationThreadDelegate(int fd)
  : readfd_(fd),
    heart_beat_sender_(
      new timer::TimerThread<ApplinkNotificationThreadDelegate>(
        "AppLinkHearBeat",
        this,
        &ApplinkNotificationThreadDelegate::sendHeartBeat, true)),
      heart_beat_timeout_(profile::Profile::instance()->hmi_heart_beat_timeout()) {

  attributes_.mq_maxmsg = MSGQ_MAX_MESSAGES;
  attributes_.mq_msgsize = MAX_QUEUE_MSG_SIZE;
  attributes_.mq_flags = 0;

  init_mq(PREFIX_STR_FROMSDLHEARTBEAT_QUEUE, O_RDWR | O_CREAT, mq_from_sdl_);
}

ApplinkNotificationThreadDelegate::~ApplinkNotificationThreadDelegate() {
  close_mq(mq_from_sdl_);
}

void ApplinkNotificationThreadDelegate::threadMain() {

  char buffer[MAX_QUEUE_MSG_SIZE];
  ssize_t length = 0;

#if defined __QNX__
  // Policy initialization
  utils::System policy_init(kShellInterpreter);
  policy_init.Add(kPolicyInitializationScript);
  if (!policy_init.Execute(true)) {
    LOG4CXX_FATAL(logger_, "Failed QDB initialization");
    DEINIT_LOGGER();
    exit(EXIT_FAILURE);
  }
#endif

  sem_t *sem;
  while (!g_bTerminate) {
    if ( (length = read(readfd_, buffer, sizeof(buffer))) != -1) {
      switch (buffer[0]) {
        case SDL_MSG_SDL_START:
          startSmartDeviceLink();
          if (0 < heart_beat_timeout_) {
            heart_beat_sender_->start(heart_beat_timeout_);
          }
          break;
        case SDL_MSG_START_USB_LOGGING:
          startUSBLogging();
          break;
        case SDL_MSG_SDL_STOP:
          stopSmartDeviceLink();
          LOG4CXX_INFO(logger_, "Application stopped due SDL_MSG_SDL_STOP");
          DEINIT_LOGGER();
          exit(EXIT_SUCCESS);
          break;
        case SDL_MSG_LOW_VOLTAGE:
          main_namespace::LifeCycle::instance()->LowVoltage();
          sem = sem_open("/SDLSleep", O_RDWR);
          if (!sem) {
            fprintf(stderr, "Error opening semaphore: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
          }
          sem_post(sem);
          sem_close(sem);
          break;
        case SDL_MSG_WAKE_UP:
          main_namespace::LifeCycle::instance()->WakeUp();
          break;
        default:
          break;
      }
    }
  } //while-end
}

void ApplinkNotificationThreadDelegate::init_mq(const std::string& name,
                                                int flags,
                                                mqd_t& mq_desc) {

  mq_desc = mq_open(name.c_str(), flags, 0666, &attributes_);
  if (-1 == mq_desc) {
    LOG4CXX_ERROR(logger_, "Unable to open mq " << name.c_str() << " : " << strerror(errno));
  }
}

void ApplinkNotificationThreadDelegate::close_mq(mqd_t mq_to_close) {
  if (-1 == mq_close(mq_to_close)) {
    LOG4CXX_ERROR(logger_, "Unable to close mq: " << strerror(errno));
  }
}

void ApplinkNotificationThreadDelegate::sendHeartBeat() {
  if (-1 != mq_from_sdl_) {
    char buf[MAX_QUEUE_MSG_SIZE];
    buf[0] = SDL_MSG_HEARTBEAT_ACK;
    if (-1 == mq_send(mq_from_sdl_, &buf[0], MAX_QUEUE_MSG_SIZE, 0)) {
      LOG4CXX_ERROR(logger_, "Unable to send heart beat via mq: " << strerror(errno));
    } else {
      LOG4CXX_DEBUG(logger_, "heart beat to applink has been sent");
    }
  }
}

class Dispatcher {
 public:
  Dispatcher(int pipefd, int pid)
      : state_(kRun), pipefd_(pipefd), pid_(pid) {}
  void Process(const std::string& msg) {
    if (msg.empty()) {
      fprintf(stderr, "Error: message is empty\n");
      return;
    }
    char code = msg[0];
    switch (state_) {
      case kRun:
        if (code == SDL_MSG_LOW_VOLTAGE) {
          OnLowVoltage(msg);
          Signal(SIGSTOP);
          state_ = kSleep;
        } else if (code == SDL_MSG_SDL_STOP) {
          Send(msg);
          state_ = kStop;
        } else if (code != SDL_MSG_WAKE_UP) {
          Send(msg);
        }
        break;
      case kSleep:
        if (code == SDL_MSG_WAKE_UP) {
          Signal(SIGCONT);
          Send(msg);
          state_ = kRun;
        } else if (code == SDL_MSG_SDL_STOP) {
          Signal(SIGCONT);
          Send(msg);
          state_ = kStop;
        }
        break;
      case kStop: /* nothing do here */ break;
    }
  }
  bool IsActive() const {
    return state_ != kStop;
  }
 private:
  enum State { kRun, kSleep, kStop };
  State state_;
  int pipefd_;
  int pid_;
  void OnLowVoltage(const std::string& msg) {
    sem_t *sem = sem_open("/SDLSleep", O_CREAT | O_RDONLY, 0666, 0);
    if (!sem) {
      fprintf(stderr, "Error opening semaphore: %s\n", strerror(errno));
      exit(EXIT_FAILURE);
    }
    Send(msg);
    sem_wait(sem);
    sem_close(sem);
  }
  void Signal(int sig) {
    if (kill(pid_, sig) == -1) {
      fprintf(stderr, "Error sending %s signal: %s\n",
              strsignal(sig), strerror(errno));
    }
  }
  void Send(const std::string& msg) {
    write(pipefd_, msg.c_str(), msg.length());
  }
};

class MQueue {
 public:
  MQueue(): mq_(-1) {
    struct mq_attr attributes;
    attributes.mq_maxmsg = MSGQ_MAX_MESSAGES;
    attributes.mq_msgsize = MAX_QUEUE_MSG_SIZE;
    attributes.mq_flags = 0;

    mq_ = mq_open(PREFIX_STR_SDL_PROXY_QUEUE,
                  O_RDONLY | O_CREAT,
                  S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH,
                  &attributes);
  }
  ~MQueue() {
    mq_close(mq_);
  }
  std::string Receive() {
    ssize_t length = mq_receive(mq_, buffer_, sizeof(buffer_), 0);
    if (length == -1) {
      fprintf(stderr, "Error receiving message: %s\n", strerror(errno));
      return "";
    }
    return std::string(buffer_, length);
  }

 private:
  mqd_t mq_;
  char buffer_[MAX_QUEUE_MSG_SIZE];
};

/**
 * \brief Entry point of the program.
 * \param argc number of argument
 * \param argv array of arguments
 * \return EXIT_SUCCESS
 */
int main(int argc, char** argv) {

  int pipefd[2];
  if (pipe(pipefd) != 0) {
    fprintf(stderr, "Error creating pipe: %s", strerror(errno));
    exit(EXIT_FAILURE);
  }

  int pid  = getpid();
  int cpid = fork();

  if (cpid < 0) {
    fprintf(stderr, "Error due fork() call: %s", strerror(errno));
    exit(EXIT_FAILURE);
  }

  if (cpid == 0) {
    // Child process reads mqueue, translates all received messages to the pipe
    // and reacts on some of them (e.g. SDL_MSG_LOW_VOLTAGE)
    
    // rename child process
    int argv_size = strlen(argv[0]);
    strncpy(argv[0],"SDLDispatcher",argv_size);

    close(pipefd[0]);

    MQueue queue;
    Dispatcher dispatcher(pipefd[1], pid);
    while (dispatcher.IsActive()) {
      std::string msg = queue.Receive();
      dispatcher.Process(msg);
    }

    close(pipefd[1]);
    exit(EXIT_SUCCESS);
  }
  close(pipefd[1]);

  profile::Profile::instance()->config_file_name(SDL_INIFILE_PATH);
  INIT_LOGGER(profile::Profile::instance()->log4cxx_config_file());
  configureLogging();

  LOG4CXX_INFO(logger_, "Snapshot: {TAG}");
  LOG4CXX_INFO(logger_, "Git commit: {GIT_COMMIT}");
  LOG4CXX_INFO(logger_, "Application main()");

  if (!utils::appenders_loader.Loaded()) {
    LOG4CXX_ERROR(logger_, "Appenders plugin not loaded, file logging disabled");
  }

  threads::Thread* applink_notification_thread =
      threads::CreateThread("ApplinkNotify", new ApplinkNotificationThreadDelegate(pipefd[0]));
  applink_notification_thread->start();

  main_namespace::LifeCycle::instance()->Run();

  LOG4CXX_INFO(logger_, "Stopping application due to signal caught");
  stopSmartDeviceLink();

  LOG4CXX_INFO(logger_, "Waiting SDL controller finished.");
  close(pipefd[0]);
  int result;
  waitpid(cpid, &result, 0);

  LOG4CXX_INFO(logger_, "Application successfully stopped");
  DEINIT_LOGGER();
  return EXIT_SUCCESS;
}
///EOF
