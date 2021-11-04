#include "sylar/sylar.h"
#include "yaml-cpp/yaml.h"

sylar::Logger::ptr test_thread_logger = SYLAR_LOG_ROOT();

sylar::RWMutex s_mutex;
int count = 0;

void fun1() {
  SYLAR_LOG_DEBUG(test_thread_logger) << "name: " << sylar::Thread::GetName()
                                      << ", this.name: " << sylar::Thread::GetThis()->getName()
                                      << ", id: " << sylar::GetThreadId()
                                      << ", this.id: " << sylar::Thread::GetThis()->getId();

  for (int i = 0; i < 500000; ++i) {
    // sylar::RWMutex::ReadLock lock(s_mutex);
    ++count;
  }
}

void func2() {
  while (count < 5000000) {
    SYLAR_LOG_DEBUG(test_thread_logger) << "********************************************";
    ++count;
  }
}

void func3() {
  while (count < 5000000) {
    SYLAR_LOG_DEBUG(test_thread_logger) << "============================================";
    ++count;
  }
}

/// 无锁  5000000 15057289 16s 14条错误数据
/// Mutex 5000000 5000025 21s 无错误数据 
/// SpinLock 5000000 5000125 18s 无错误数据
/// CASLock  5000000 5000031 15s 无错误数据
void test_thread() {
  SYLAR_LOG_DEBUG(test_thread_logger) << "TEST THREAD BEGIN";
  std::vector<sylar::Thread::ptr> thread_ptr_vec;
  for (size_t i = 0; i < 5; ++i) {
    sylar::Thread::ptr thread_ptr(new sylar::Thread(&fun1, "name_" + std::to_string(i)));
    thread_ptr_vec.push_back(thread_ptr);
  }

  for (auto &thread_item : thread_ptr_vec) {
    thread_item->join();
  }
  SYLAR_LOG_DEBUG(test_thread_logger) << count;
  SYLAR_LOG_DEBUG(test_thread_logger) << "TEST THREAD END";
}

void test_mutex() {
  int start_time = time(0);
  std::cout << start_time << std::endl;
  SYLAR_LOG_DEBUG(test_thread_logger) << "TEST MUTEX BEGIN";
  YAML::Node root = YAML::LoadFile("/home/wangkc/demo/sylar/bin/conf/log2.yaml");
  sylar::Config::LoadFromYaml(root);

  std::vector<sylar::Thread::ptr> thread_ptr_vec;
  for (int i = 0; i < 2; ++i) {
    sylar::Thread::ptr thread_ptr_1(new sylar::Thread(&func2, "name_" + std::to_string(i)));
    sylar::Thread::ptr thread_ptr_2(new sylar::Thread(&func3, "name_" + std::to_string(i)));
    thread_ptr_vec.push_back(thread_ptr_1);
    thread_ptr_vec.push_back(thread_ptr_2);
  }

  for (auto &thread_item : thread_ptr_vec) {
    thread_item->join();
  }

  SYLAR_LOG_DEBUG(test_thread_logger) << "TEST MUTEX END";
  int ent_time = time(0);
  std::cout << ent_time - start_time << std::endl;
}

int main(int argc, char **argv) {
  test_mutex();
  return 0;
}