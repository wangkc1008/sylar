#include "util.h"

#include <execinfo.h>

#include "log.h"

namespace sylar {
sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

pid_t GetThreadId() { return syscall(SYS_gettid); }
uint32_t GetFiberId() { return 0; }

void Backtrace(std::vector<std::string> &vec, int size, int skip) {
  void **array = (void **)malloc(sizeof(void *) * size);
  int s = ::backtrace(array, size);

  char **strings = ::backtrace_symbols(array, s);
  if (!strings) {
    SYLAR_LOG_ERROR(g_logger) << "backtrace_symbols error";
    return;
  }

  for (int i = skip; i < s; ++i) {
    vec.push_back(strings[i]);
  }
}

std::string BacktraceTostring(int size, int skip, const std::string &prefix) {
  std::vector<std::string> vec;
  Backtrace(vec, size, skip);
  std::stringstream ss;
  for (auto &i : vec) {
    ss << prefix << i << std::endl;
  }
  return ss.str();
}
}  // namespace sylar