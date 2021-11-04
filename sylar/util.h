#ifndef __SYLAR_UTIL_H__
#define __SYALR_UTIL_H__

#include <stdint.h>
#include <string>
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

namespace sylar {
pid_t GetThreadId();
uint32_t GetFiberId();
void Backtrace(std::vector<std::string> &vec, int size, int skip);
std::string BacktraceTostring(int size, int skip, const std::string &prefix);
}  // namespace sylar

#endif  // __SYALR_UTIL_H__