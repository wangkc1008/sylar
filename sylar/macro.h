#include <execinfo.h>

#include "log.h"
#include "util.h"

#define SYLAR_ASSERT(x)                                                            \
  if (!(x)) {                                                                      \
    SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "Assertion: " << #x << "\nBackTrace:\n"   \
                                      << sylar::BacktraceTostring(100, 2, "    "); \
    assert(x);                                                                     \
  }

#define SYLAR_ASSERT_P(x, w)                                                       \
  if (!(x)) {                                                                      \
    SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "Assertion:" << #x << "\n"                \
                                      << w << "\nBackTrace:\n"                     \
                                      << sylar::BacktraceTostring(100, 2, "    "); \
    assert(x);                                                                     \
  }
