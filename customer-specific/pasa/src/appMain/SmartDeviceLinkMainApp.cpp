#include <string.h>
#include <log4cxx/rollingfileappender.h>
#include <log4cxx/fileappender.h>

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
  // TODO (dchmerev@luxoft.com): moderate requirements to value (it hasn't to be exactly "0K" to be zero)
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
    LOG4CXX_INFO(logger_, "StartComponents failed.");
#ifdef ENABLE_LOG
    logger::LogMessageLoopThread::destroy();
#endif
    DEINIT_LOGGER();
    exit(EXIT_FAILURE);
  }

  // --------------------------------------------------------------------------
  // Third-Party components initialization.

  if (!main_namespace::LifeCycle::instance()->InitMessageSystem()) {
    LOG4CXX_INFO(logger_, "InitMessageBroker failed");
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
  ApplinkNotificationThreadDelegate(int fd)
    : readfd_(fd) { }
  virtual void threadMain();
 private:
  int readfd_;
};

void ApplinkNotificationThreadDelegate::threadMain() {

  char buffer[MAX_QUEUE_MSG_SIZE];
  ssize_t length = 0;

#if defined __QNX__
  // Policy initialization
  utils::System policy_init(kShellInterpreter);
  policy_init.Add(kPolicyInitializationScript);
  if (!policy_init.Execute(true)) {
    LOG4CXX_ERROR(logger_, "QDB initialization failed.");
#ifdef ENABLE_LOG
    logger::LogMessageLoopThread::destroy();
#endif
    DEINIT_LOGGER();
    exit(EXIT_FAILURE);
  }
#endif

  while (!g_bTerminate) {
    if ( (length = read(readfd_, buffer, sizeof(buffer))) != -1) {
      switch (buffer[0]) {
        case SDL_MSG_SDL_START:
          startSmartDeviceLink();
          break;
        case SDL_MSG_START_USB_LOGGING:
          startUSBLogging();
          break;
        case SDL_MSG_SDL_STOP:
          stopSmartDeviceLink();
#ifdef ENABLE_LOG
          logger::LogMessageLoopThread::destroy();
#endif
          DEINIT_LOGGER();
          exit(EXIT_SUCCESS);
          break;
        case SDL_MSG_LOW_VOLTAGE:
          main_namespace::LifeCycle::instance()->LowVoltage();
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

void dispatchCommands(mqd_t mqueue, int pipefd, int pid) {
  char buffer[MAX_QUEUE_MSG_SIZE];
  ssize_t length = 0;

  while (!g_bTerminate) {
    if ( (length = mq_receive(mqueue, buffer, sizeof(buffer), 0)) != -1) {
      switch (buffer[0]) {
        case SDL_MSG_LOW_VOLTAGE:
          if(kill(pid, SIGSTOP) == -1) {
            fprintf(stderr, "Error sending SIGSTOP signal: %s\n", strerror(errno));
          }
          break;
        case SDL_MSG_WAKE_UP:
          if(kill(pid, SIGCONT) == -1) {
            fprintf(stderr, "Error sending SIGCONT signal: %s\n", strerror(errno));
          }
          break;
        case SDL_MSG_SDL_STOP:
          g_bTerminate = true;
          break;
      }
      write(pipefd, buffer, length);
    }
  } // while(!g_bTerminate)
}

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

  if (cpid > 0) {
    // Child process reads mqueue, translates all received messages to the pipe
    // and reacts on some of them (e.g. SDL_MSG_LOW_VOLTAGE)
    close(pipefd[0]);
    struct mq_attr attributes;
    attributes.mq_maxmsg = MSGQ_MAX_MESSAGES;
    attributes.mq_msgsize = MAX_QUEUE_MSG_SIZE;
    attributes.mq_flags = 0;

    mqd_t mq = mq_open(PREFIX_STR_SDL_PROXY_QUEUE,
                       O_RDONLY | O_CREAT,
                       S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH,
                       &attributes);

    dispatchCommands(mq, pipefd[1], pid);

    close(pipefd[1]);
    exit(EXIT_SUCCESS);
    waitpid(cpid, &result, 0);
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

  close(pipefd[0]);

  LOG4CXX_INFO(logger_, "Application successfully stopped");
#ifdef ENABLE_LOG
  logger::LogMessageLoopThread::destroy();
#endif
  DEINIT_LOGGER();
  return EXIT_SUCCESS;
}


///EOF
