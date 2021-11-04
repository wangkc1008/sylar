#ifndef __SYLAR_LOG_H__
#define __SYLAR_LOG_H__

#include <fstream>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <sstream>
#include <stdarg.h>
#include <string>
#include <vector>

#include "thread.h"
#include "singleton.h"

// 只有高于设置的Level的日志才会被输出
#define SYLAR_LOG_LEVEL(logger, level)                                                                               \
  if (logger->getLevel() <= level)                                                                                   \
  sylar::LogEventWrap(sylar::LogEvent::ptr(new sylar::LogEvent(logger, level, __FILE__, __LINE__, 0,                 \
                                                               sylar::GetThreadId(), sylar::GetFiberId(), time(0)))) \
    .getSS()

#define SYLAR_LOG_DEBUG(logger) SYLAR_LOG_LEVEL(logger, sylar::LogLevel::DEBUG)
#define SYLAR_LOG_INFO(logger) SYLAR_LOG_LEVEL(logger, sylar::LogLevel::INFO)
#define SYLAR_LOG_WARN(logger) SYLAR_LOG_LEVEL(logger, sylar::LogLevel::WARN)
#define SYLAR_LOG_ERROR(logger) SYLAR_LOG_LEVEL(logger, sylar::LogLevel::ERROR)
#define SYLAR_LOG_FATAL(logger) SYLAR_LOG_LEVEL(logger, sylar::LogLevel::FATAL)

#define SYLAR_LOG_FMT_LEVEL(logger, level, fmt, ...)                                                                 \
  if (logger->getLevel() <= level)                                                                                   \
  sylar::LogEventWrap(sylar::LogEvent::ptr(new sylar::LogEvent(logger, level, __FILE__, __LINE__, 0,                 \
                                                               sylar::GetThreadId(), sylar::GetFiberId(), time(0)))) \
    .getEvent()                                                                                                      \
    ->format(fmt, __VA_ARGS__)

#define SYLAR_LOG_FMT_DEBUG(logger, fmt, ...) SYLAR_LOG_FMT_LEVEL(logger, sylar::LogLevel::DEBUG, fmt, __VA_ARGS__)
#define SYLAR_LOG_FMT_INFO(logger, fmt, ...) SYLAR_LOG_FMT_LEVEL(logger, sylar::LogLevel::INFO, fmt, __VA_ARGS__)
#define SYLAR_LOG_FMT_WARN(logger, fmt, ...) SYLAR_LOG_FMT_LEVEL(logger, sylar::LogLevel::WARN, fmt, __VA_ARGS__)
#define SYLAR_LOG_FMT_ERROR(logger, fmt, ...) SYLAR_LOG_FMT_LEVEL(logger, sylar::LogLevel::ERROR, fmt, __VA_ARGS__)
#define SYLAR_LOG_FMT_FATAL(logger, fmt, ...) SYLAR_LOG_FMT_LEVEL(logger, sylar::LogLevel::FATAL, fmt, __VA_ARGS__)

#define SYLAR_LOG_ROOT() sylar::LoggerMgr::getInstance()->getRoot()
#define SYLAR_LOG_NAME(name) sylar::LoggerMgr::getInstance()->getLogger(name)

namespace sylar {

class Logger;
class LogManager;

// 日志等级
class LogLevel {
 public:
  enum Level { UNKNOW = 0, DEBUG = 1, INFO = 2, WARN = 3, ERROR = 4, FATAL = 5 };

  static const char *ToString(LogLevel::Level level);
  static LogLevel::Level fromString(const std::string &str);
  static std::string toString(const LogLevel::Level &level);
};

// 日志事件
class LogEvent {
 public:
  typedef std::shared_ptr<LogEvent> ptr;
  LogEvent(std::shared_ptr<Logger> logger, LogLevel::Level level, const char *file, int32_t line, uint32_t elapse,
           uint32_t threadId, uint32_t fiberId, uint64_t time);

  const char *getFile() const { return m_file; }
  int32_t getLine() const { return m_line; }
  uint32_t getElapse() const { return m_elapse; }
  uint32_t getThreadId() const { return m_threadId; }
  uint32_t getFiberId() const { return m_fiberId; }
  uint64_t getTime() const { return m_time; }
  std::string getContent() const { return m_ss.str(); }
  std::stringstream &getSS() { return m_ss; }

  std::shared_ptr<Logger> getLogger() const { return m_logger; }
  LogLevel::Level getLevel() const { return m_level; }

  void format(const char *fmt, ...);
  void format(const char *fmt, va_list vl);

 private:
  const char *m_file = nullptr;  // 文件名
  int32_t m_line = 0;            // 行号
  uint32_t m_elapse = 0;         // 程序启动到现在的毫秒
  uint32_t m_threadId = 0;       // 线程ID
  uint32_t m_fiberId = 0;        // 协程ID
  uint64_t m_time;               // 时间戳
  std::stringstream m_ss;        // 内容

  std::shared_ptr<Logger> m_logger;
  LogLevel::Level m_level;
};

class LogEventWrap {  // LogEvent包装器
 public:
  LogEventWrap(LogEvent::ptr event);
  ~LogEventWrap();  // 用来在析构时执行logger->log()

  std::ostream &getSS();
  std::shared_ptr<LogEvent> getEvent();

 private:
  LogEvent::ptr m_event;
};

// 日志格式器
class LogFormatter {
 public:
  typedef std::shared_ptr<LogFormatter> ptr;
  LogFormatter(const std::string &pattern);

  std::string format(std::shared_ptr<Logger> logger, LogLevel::Level level,
                     LogEvent::ptr event);  // 将LogEvent格式化成string输出
 public:
  class FormatItem {
   public:
    typedef std::shared_ptr<FormatItem> ptr;
    virtual ~FormatItem() {}  // 虚析构
    virtual void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level,
                        LogEvent::ptr event) = 0;  // 纯虚函数 存在纯虚函数就是抽象类 被继承
  };

  bool isError() const { return m_error; }
  const std::string &getPattern() { return m_pattern; }

  // 日志格式的解析
  void init();

 private:
  std::string m_pattern;
  std::vector<FormatItem::ptr> m_items;
  bool m_error = false;
};

// 日志输出地
class LogAppender {
  friend Logger;

 public:
  typedef std::shared_ptr<LogAppender> ptr;
  typedef CASLock MutexType;
  virtual ~LogAppender() {}  // 虚析构 delete指向子类的基类指针时防止子类数据不会被释放而造成的内存泄漏
                             // 既会释放基类对象也会释放子类对象 否则只会释放基类对象

  virtual void log(std::shared_ptr<Logger> logger, LogLevel::Level level,
                   LogEvent::ptr event) = 0;  // 纯虚函数，子类必须实现
  virtual std::string toYamlString() = 0;

  void setFormatter(LogFormatter::ptr formatter);
  LogFormatter::ptr getFormatter();
  void setLevel(LogLevel::Level level) { m_level = level; }
  LogLevel::Level getLevel() const { return m_level; }

 protected:                                   // 设置成protected方便子类继承
  LogLevel::Level m_level = LogLevel::DEBUG;  // 当前appender针对哪类日志
  LogFormatter::ptr m_formatter;              // event的日志格式
  MutexType m_mutex;  
  bool m_hasFormatter = false;
};

// 日志器
class Logger : public std::enable_shared_from_this<Logger> {  // 用来获取指向自己的智能指针
  friend LogManager;

 public:
  typedef std::shared_ptr<Logger> ptr;
  typedef CASLock MutexType;

  Logger(const std::string &name = "root");
  void log(LogLevel::Level level, LogEvent::ptr event);

  void debug(LogEvent::ptr event);
  void info(LogEvent::ptr event);
  void warn(LogEvent::ptr event);
  void error(LogEvent::ptr event);
  void fatal(LogEvent::ptr event);

  void addAppender(LogAppender::ptr appender);
  void delAppender(LogAppender::ptr appender);
  void clearAppenders();

  std::string toYamlString();

  LogLevel::Level getLevel() const { return m_level; }
  void setLevel(LogLevel::Level level) { m_level = level; }
  const std::string &getName() const { return m_name; }

  void setFormatter(const LogFormatter::ptr val);
  void setFormatter(const std::string &val);
  LogFormatter::ptr getFormatter();

 private:
  std::string m_name;                       // 日志名称
  LogLevel::Level m_level;                  // 日志器的日志级别
  std::list<LogAppender::ptr> m_appenders;  // Appender集合
  LogFormatter::ptr m_formatter;
  Logger::ptr m_root;
  MutexType m_mutex;
};

// 输出到控制台的Appender
class StdoutAppender : public LogAppender {
 public:
  typedef std::shared_ptr<StdoutAppender> ptr;
  void log(Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override;  // 重写实现父类的纯虚函数
  std::string toYamlString() override;
};

// 输出到文件的Appender
class FileAppender : public LogAppender {
 public:
  typedef std::shared_ptr<FileAppender> ptr;

  FileAppender(const std::string &filename);
  void log(Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override;  // 重写实现父类的纯虚函数
  std::string toYamlString() override;

  // 重新打开文件
  bool reopen();

 private:
  std::string m_filename;
  std::ofstream m_filestream;
  uint64_t m_lastTime = 0;
};

// 日志管理器
class LogManager {
 public:
  typedef CASLock MutexType;
  LogManager();
  Logger::ptr getLogger(const std::string &name);
  Logger::ptr getRoot() const { return m_root; }

  std::string toYamlString();

  void init();

 private:
  std::map<std::string, Logger::ptr> m_loggers;
  Logger::ptr m_root;
  MutexType m_mutex;
};

typedef sylar::Singleton<LogManager> LoggerMgr;

}  // namespace sylar

#endif  // __SYLAR_LOG_H__