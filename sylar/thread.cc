#include "thread.h"

#include "log.h"
#include "util.h"

namespace sylar {
static thread_local Thread *t_thread = nullptr;  // 线程局部变量
static thread_local std::string t_thread_name = "UNKNOW";

sylar::Logger::ptr thread_logger = SYLAR_LOG_NAME("system");

Semaphore::Semaphore(uint32_t count) {
  if (sem_init(&m_semaphore, 0, count)) {  // return 0 on success, on error -1 is returned
    throw std::logic_error("sem_init error");
  }
}

Semaphore::~Semaphore() {
  sem_destroy(&m_semaphore);  // returns 0 on success; on error, -1 is returned
}

void Semaphore::wait() {
  if (sem_wait(&m_semaphore)) {  // 信号量减1 returns 0 on success; on error, -1 is returned
    throw std::logic_error("sem_wait error");
  }
}

void Semaphore::notify() {
  if (sem_post(&m_semaphore)) {  // 信号量加1 returns 0 on success; on error, the value of the semaphore is left
                                 // unchanged, -1 is returned
    throw std::logic_error("sem_post error");
  }
}

Thread *Thread::GetThis() { return t_thread; }

const std::string &Thread::GetName() { return t_thread_name; }

void Thread::SetName(const std::string &name) {
  if (t_thread) {
    t_thread->m_name = name;
  }
  t_thread_name = name;
}

Thread::Thread(std::function<void()> cb, const std::string &name) : m_cb(cb), m_name(name) {
  if (name.empty()) {
    m_name = "UNKNOW";
  }
  int res = pthread_create(&m_thread, nullptr, &Thread::run, this);
  if (res) {
    SYLAR_LOG_ERROR(thread_logger) << "pthread_create thread fail, res=" << res << ", name=" << m_name;
    throw std::logic_error("pthread_create error");
  }
  m_semaphore.wait();  // 直到run的时候才真正执行
}

Thread::~Thread() {
  if (m_thread) {
    pthread_detach(m_thread);
  }
}

void Thread::join() {
  if (m_thread) {
    int res = pthread_join(m_thread, nullptr);
    if (res) {
      SYLAR_LOG_ERROR(thread_logger) << "pthread_join thread fail, res=" << res << ", name=" << m_name;
      throw std::logic_error("pthread_join error");
    }
    m_thread = 0;
  }
}

void *Thread::run(void *arg) {
  Thread *thread = (Thread *)arg;
  t_thread = thread;
  t_thread_name = thread->m_name;
  thread->m_id = GetThreadId();
  pthread_setname_np(pthread_self(), thread->m_name.substr(0, 15).c_str());

  std::function<void()> cb;
  cb.swap(thread->m_cb);  // 不会增加智能指针的引用
  
  thread->m_semaphore.notify();  // 保证线程类创建成功之后启动线程
  
  cb();
  return 0;
}
}  // namespace sylar