#include <chrono>
#include <thread>

#include "automaton/core/io/io.h"

void sleep(uint32_t x) {
  std::this_thread::sleep_for(std::chrono::milliseconds(x));
}

struct Test {
  void log_test(int x) {
    LOG(WARNING) << "This is warning from the log_test(" << x << ")";
  }
};

void performance_test(int iter) {
  TIMED_FUNC(timerObj);
  // Some initializations
  // Some more heavy tasks
  sleep(10);
  while (iter-- > 0) {
    TIMED_SCOPE(timerBlkObj, "heavy-iter");
    // Perform some heavy task in each iter
    sleep(20);
    if (iter % 5 == 0) {
      PERFORMANCE_CHECKPOINT(timerBlkObj);
    }
  }
  el::base::debug::StackTrace();
}

int main() {
  el::Logger* log_core_data = el::Loggers::getLogger("core.data");
  el::Logger* log_core_script = el::Loggers::getLogger("core.script");

  el::Loggers::addFlag(el::LoggingFlag::ColoredTerminalOutput);

  // el::Loggers::addFlag(el::LoggingFlag::HierarchicalLogging);
  // el::Loggers::setLoggingLevel(el::Level::Trace);
  // el::Loggers::setLoggingLevel(el::Level::Info);
  el::Loggers::setLoggingLevel(el::Level::Global);
  el::Loggers::setVerboseLevel(9);

  LOG(TRACE) << "This is trace log!";
  LOG(DEBUG) << "This is debug log!";
  LOG(INFO) << "This is info log!";
  LOG(WARNING) << "This is warning log!";
  LOG(ERROR) << "This is error log!";
  VLOG(0) << "This is verbose level 0 log!";
  VLOG(1) << "This is verbose level 1 log!";
  VLOG(2) << "This is verbose level 2 log!";
  VLOG(3) << "This is verbose level 3 log!";
  VLOG(4) << "This is verbose level 4 log!";
  VLOG(5) << "This is verbose level 5 log!";
  VLOG(6) << "This is verbose level 6 log!";
  VLOG(7) << "This is verbose level 7 log!";
  VLOG(8) << "This is verbose level 8 log!";
  VLOG(9) << "This is verbose level 9 log!";

  performance_test(10);

  log_core_data->info("Data module loaded.");
  log_core_script->error("Script up and running!");

  Test test;

  test.log_test(15);

  return 0;
}
