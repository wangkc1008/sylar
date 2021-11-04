#ifndef __SYLAR_THREAD_H__
#define __SYLAR_THREAD_H__

#include <atomic>
#include <functional>
#include <memory>
#include <pthread.h>
#include <semaphore.h>
#include <thread>

namespace sylar {
// 条件量
class Semaphore {
 public:
  Semaphore(uint32_t count = 0);
  ~Semaphore();

  void wait();
  void notify();

 private:
  Semaphore(const Semaphore &) = delete;
  Semaphore(const Semaphore &&) = delete;
  Semaphore &operator=(const Semaphore &) = delete;

 private:
  sem_t m_semaphore;
};

// 防止漏掉互斥量的加锁和解锁 使用类的构造函数加锁 析构函数解锁
// 在某{}内实例化该对象时加锁 根据局部变量的作用域和生命周期 离开该{}时会调用该对象的析构函数实现解锁
template <typename T>
class ScopedLockImpl {
 public:
  ScopedLockImpl(T &mutex) : m_mutex(mutex) {
    m_mutex.lock();
    m_locked = true;
  }

  ~ScopedLockImpl() { unlock(); }

  void lock() {
    if (!m_locked) {
      m_mutex.lock();
      m_locked = true;
    }
  }

  void unlock() {
    if (m_locked) {
      m_mutex.unlock();
      m_locked = false;
    }
  }

 private:
  T &m_mutex;
  bool m_locked;
};

// 防止漏掉读锁的加锁和解锁 使用类的构造函数加锁 析构函数解锁
template <typename T>
class ReadScopedLockImpl {
 public:
  ReadScopedLockImpl(T &mutex) : m_mutex(mutex) {
    m_mutex.rdlock();
    m_locked = true;
  }

  ~ReadScopedLockImpl() { unlock(); }

  void lock() {
    if (!m_locked) {
      m_mutex.rdlock();
      m_locked = true;
    }
  }

  void unlock() {
    if (m_locked) {
      m_mutex.unlock();
      m_locked = false;
    }
  }

 private:
  T &m_mutex;
  bool m_locked;
};

// 防止漏掉写锁的加锁和解锁 使用类的构造函数加锁 析构函数解锁
template <typename T>
class WriteScopedLockImpl {
 public:
  WriteScopedLockImpl(T &mutex) : m_mutex(mutex) {
    m_mutex.wrlock();
    m_locked = true;
  }

  ~WriteScopedLockImpl() { unlock(); }

  void lock() {
    if (!m_locked) {
      m_mutex.wrlock();
      m_locked = true;
    }
  }

  void unlock() {
    if (m_locked) {
      m_mutex.unlock();
      m_locked = false;
    }
  }

 private:
  T &m_mutex;
  bool m_locked;
};

// 互斥量
class Mutex {
 public:
  typedef ScopedLockImpl<Mutex> Lock;
  Mutex() { pthread_mutex_init(&m_mutex, nullptr); }

  ~Mutex() { pthread_mutex_destroy(&m_mutex); }

  void lock() { pthread_mutex_lock(&m_mutex); }

  void unlock() { pthread_mutex_unlock(&m_mutex); }

 private:
  pthread_mutex_t m_mutex;
};

// 自旋锁
class SpinLock {
public:
    typedef ScopedLockImpl<SpinLock> Lock;
    SpinLock() {
        pthread_spin_init(&m_mutex, 0);
    }

    ~SpinLock() {
        pthread_spin_destroy(&m_mutex);
    }

    void lock() {
        pthread_spin_lock(&m_mutex);
    }

    void unlock() {
        pthread_spin_unlock(&m_mutex);
    }
private:
    pthread_spinlock_t m_mutex;
};

class CASLock {
public:
    typedef ScopedLockImpl<CASLock> Lock;
    CASLock() {
        m_mutex.clear();
    }

    ~CASLock() {
    }

    void lock() {
        while (std::atomic_flag_test_and_set_explicit(&m_mutex, std::memory_order_acquire));
    }

    void unlock() {
        std::atomic_flag_clear_explicit(&m_mutex, std::memory_order_release);
    }
private:
    volatile std::atomic_flag m_mutex;  // 该变量可能在外部改变，不能在寄存器中缓存该变量，每次使用时必须从内存地址读取
};

class NullMutex {
public:
    typedef ScopedLockImpl<NullMutex> Lock;
    NullMutex() {}
    ~NullMutex() {}
    void lock() {}
    void unlock() {}
};

// 读写锁的互斥量
class RWMutex {
 public:
  typedef ReadScopedLockImpl<RWMutex> ReadLock;
  typedef WriteScopedLockImpl<RWMutex> WriteLock;

  RWMutex() { pthread_rwlock_init(&m_lock, nullptr); }

  ~RWMutex() { pthread_rwlock_destroy(&m_lock); }

  void rdlock() { pthread_rwlock_rdlock(&m_lock); }

  void wrlock() { pthread_rwlock_wrlock(&m_lock); }

  void unlock() { pthread_rwlock_unlock(&m_lock); }

 private:
  pthread_rwlock_t m_lock;
};

class NullRWMutex {
public:
    typedef ReadScopedLockImpl<NullRWMutex> ReadLock;
    typedef WriteScopedLockImpl<NullRWMutex> WriteLock;
    NullRWMutex() {}
    ~NullRWMutex() {}
    void rdlock() {}
    void wrlock() {}
    void unlock() {}
};

class Thread {
 public:
  typedef std::shared_ptr<Thread> ptr;
  Thread(std::function<void()> cb, const std::string &name);
  ~Thread();

  pid_t getId() const { return m_id; }
  const std::string &getName() const { return m_name; }

  void join();

  static Thread *GetThis();
  static const std::string &GetName();
  static void SetName(const std::string &name);

 private:
  Thread(const Thread &) = delete;             // 禁止拷贝构造
  Thread(const Thread &&) = delete;            // 禁止移动拷贝构造
  Thread &operator=(const Thread &) = delete;  // 禁止赋值运算符重载

  static void *run(void *arg);

 private:
  pid_t m_id = -1;
  pthread_t m_thread = 0;  // long int类型
  std::function<void()> m_cb;
  std::string m_name;
  Semaphore m_semaphore;
};
}  // namespace sylar
#endif  // __SYLAR_THREAD_H__
