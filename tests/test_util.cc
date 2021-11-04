#include "sylar/sylar.h"

#include <assert.h>

sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

void test_assert() {
  // SYLAR_LOG_INFO(g_logger) << sylar::BacktraceTostring(10, 2, "*******");
  SYLAR_ASSERT_P(1 <= 0, "haha");
}

int main(int argc, char **argv) {
  test_assert();
  return 0;
}