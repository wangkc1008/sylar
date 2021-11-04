#include "log.h"

#include <functional>
#include <map>
#include <tuple>

#include "config.h"
#include "util.h"

namespace sylar {

const char *LogLevel::ToString(LogLevel::Level level) {
  switch (level) {
#define XX(name)       \
  case LogLevel::name: \
    return #name;      \
    break;
    XX(DEBUG);
    XX(INFO);
    XX(WARN);
    XX(ERROR);
    XX(FATAL);
#undef XX
  default:
    return "UNKNOW";
  }
  return "UNKNOW";
}

LogLevel::Level LogLevel::fromString(const std::string &str) {
#define XX(name, uncased) \
  if (str == #uncased) return LogLevel::Level::name;
  XX(DEBUG, debug);
  XX(INFO, info);
  XX(WARN, warn);
  XX(ERROR, error);
  XX(FATAL, fatal);

  XX(DEBUG, DEBUG);
  XX(INFO, INFO);
  XX(WARN, WARN);
  XX(ERROR, ERROR);
  XX(FATAL, FATAL);
  return LogLevel::Level::UNKNOW;
#undef XX
}

std::string LogLevel::toString(const LogLevel::Level &level) {
#define XX(name) \
  if (level == LogLevel::Level::name) return #name;
  XX(DEBUG);
  XX(INFO);
  XX(WARN);
  XX(ERROR);
  XX(FATAL);
  return "UNKNOW";
#undef XX
}

/**
 *  %m 消息体
 *  %p level
 *  %c 日志名称
 *  %t 线程id
 *  %n 回车换行
 *  %d 时间
 *  %f 文件名
 *  %l 行号
 */
class MessageFormatItem : public LogFormatter::FormatItem {
 public:
  MessageFormatItem(const std::string &str = "") {}
  void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override {
    os << event->getContent();
  }
};

class LevelFormatItem : public LogFormatter::FormatItem {
 public:
  LevelFormatItem(const std::string &str = "") {}
  void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override {
    os << LogLevel::ToString(level);
  }
};

class NameFormatItem : public LogFormatter::FormatItem {
 public:
  NameFormatItem(const std::string &str = "") {}
  void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override {
    os << event->getLogger()->getName();  // event中是最原始的logger 否则可能是root写的
  }
};

class ThreadIdFormatItem : public LogFormatter::FormatItem {
 public:
  ThreadIdFormatItem(const std::string &str = "") {}
  void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override {
    os << event->getThreadId();
  }
};

class FiberIdFormatItem : public LogFormatter::FormatItem {
 public:
  FiberIdFormatItem(const std::string &str = "") {}
  void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override {
    os << event->getFiberId();
  }
};

class ElapseFormatItem : public LogFormatter::FormatItem {
 public:
  ElapseFormatItem(const std::string &str = "") {}
  void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override {
    os << event->getElapse();
  }
};

class DateTimeFormatItem : public LogFormatter::FormatItem {
 public:
  DateTimeFormatItem(const std::string &format = "%Y-%m-%d %H:%M:%S") : m_format(format) {
    if (m_format.empty()) {
      m_format = "%Y-%m-%d %H:%M:%S";
    }
  }

  void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override {
    struct tm tm;
    time_t time = event->getTime();
    localtime_r(&time, &tm);
    char buf[64];
    strftime(buf, sizeof(buf), m_format.c_str(), &tm);
    os << buf;
  }

 private:
  std::string m_format;
};

class FilenameFormatItem : public LogFormatter::FormatItem {
 public:
  FilenameFormatItem(const std::string &str = "") {}
  void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override {
    os << event->getFile();
  }
};

class LineFormatItem : public LogFormatter::FormatItem {
 public:
  LineFormatItem(const std::string &str = "") {}
  void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override {
    os << event->getLine();
  }
};

class NewLineFormatItem : public LogFormatter::FormatItem {
 public:
  NewLineFormatItem(const std::string &str = "") {}
  void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override {
    os << std::endl;
  }
};

class StringFormatItem : public LogFormatter::FormatItem {
 public:
  StringFormatItem(const std::string &str) : m_format(str) {}
  void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override {
    os << m_format;
  }

 private:
  std::string m_format;
};

class TabFormatItem : public LogFormatter::FormatItem {
 public:
  TabFormatItem(const std::string &str = "") {}
  void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override {
    os << "\t";
  }
};

// C++类成员变量的初始化顺序与其在类中的声明顺序有关
LogEvent::LogEvent(std::shared_ptr<Logger> logger, LogLevel::Level level, const char *file, int32_t line,
                   uint32_t elapse, uint32_t threadId, uint32_t fiberId, uint64_t time)
    : m_file(file),
      m_line(line),
      m_elapse(elapse),
      m_threadId(threadId),
      m_fiberId(fiberId),
      m_time(time),
      m_logger(logger),
      m_level(level) {}

void LogEvent::format(const char *fmt, ...) {
  va_list vl;
  va_start(vl, fmt);
  format(fmt, vl);
  va_end(vl);
}

void LogEvent::format(const char *fmt, va_list vl) {
  char *buf = nullptr;
  int len = vasprintf(&buf, fmt, vl);  // 不需要识别参数的个数和每个参数的类型 只需要将可变列表拷贝到缓冲区
  if (len != -1) {                     // va_arg则是通过设置的参数类型依次获取各个参数
    m_ss << std::string(buf, len);
    free(buf);
  }
}

LogEventWrap::LogEventWrap(LogEvent::ptr event) : m_event(event) {}

LogEventWrap::~LogEventWrap() { m_event->getLogger()->log(m_event->getLevel(), m_event); }

std::ostream &LogEventWrap::getSS() { return m_event->getSS(); }

std::shared_ptr<LogEvent> LogEventWrap::getEvent() { return m_event; }

void LogAppender::setFormatter(LogFormatter::ptr formatter) {
  MutexType::Lock lock(m_mutex);
  m_formatter = formatter;
  if (m_formatter) {
    m_hasFormatter = true;
  } else {
    m_hasFormatter = false;
  }
}

LogFormatter::ptr LogAppender::getFormatter() {
  MutexType::Lock lock(m_mutex);
  return m_formatter;
}

Logger::Logger(const std::string &name) : m_name(name), m_level(LogLevel::DEBUG) {
  m_formatter.reset(new LogFormatter("%d{%Y-%m-%d %H:%M:%S}%T%t%T%F%T[%p]%T[%c]%T%f:%l%T%T%m%n"));
}

void Logger::addAppender(LogAppender::ptr appender) {
  MutexType::Lock lock(m_mutex);
  if (!appender->getFormatter()) {
    MutexType::Lock ll(appender->m_mutex);
    appender->m_formatter = m_formatter;  // 不改变m_hasFormatter的值 toYamlString的时候就不会输出父节点的formatter
  }
  m_appenders.push_back(appender);
}

void Logger::delAppender(LogAppender::ptr appender) {
  MutexType::Lock lock(m_mutex);
  auto it = m_appenders.begin();
  for (; it != m_appenders.end(); ++it) {
    if (*it == appender) {
      m_appenders.erase(it);
    }
  }
}

void Logger::clearAppenders() { 
  MutexType::Lock lock(m_mutex);
  m_appenders.clear();
}

void Logger::setFormatter(const LogFormatter::ptr val) {
  MutexType::Lock lock(m_mutex);
  m_formatter = val;
  // 更改父节点的formatter会影响到子节点的formatter
  for (auto& i : m_appenders) {
    // MutexType::Lock ll(i->m_mutex);
    if (!i->m_hasFormatter) {
      i->setFormatter(m_formatter);
    }
  }
}

void Logger::setFormatter(const std::string &val) {
  LogFormatter::ptr new_val(new LogFormatter(val));
  if (new_val->isError()) {
    return;
  }
  setFormatter(new_val);
}

LogFormatter::ptr Logger::getFormatter() {
  MutexType::Lock lock(m_mutex);
  return m_formatter;
}

void Logger::log(LogLevel::Level level, LogEvent::ptr event) {
  if (level >= m_level) {
    auto self = shared_from_this();
    MutexType::Lock lock(m_mutex);
    if (!m_appenders.empty()) {
      for (auto &i : m_appenders) {
        // MutexType::Lock ll(i->m_mutex);
        i->log(self, level, event);
      }
    } else if (m_root) {
      m_root->log(level, event);
    }
  }
}

std::string Logger::toYamlString() {
  MutexType::Lock lock(m_mutex);
  YAML::Node node;
  node["name"] = m_name;
  if (m_level != LogLevel::UNKNOW) {
    node["level"] = LogLevel::toString(m_level);
  }
  if (m_formatter) {
    node["formatter"] = m_formatter->getPattern();
  }
  for (auto &i : m_appenders) {
    node["appenders"].push_back(YAML::Load(i->toYamlString()));
  }
  std::stringstream ss;
  ss << node;
  return ss.str();
}

void Logger::debug(LogEvent::ptr event) { log(LogLevel::DEBUG, event); }

void Logger::info(LogEvent::ptr event) { log(LogLevel::INFO, event); }

void Logger::warn(LogEvent::ptr event) { log(LogLevel::WARN, event); }

void Logger::error(LogEvent::ptr event) { log(LogLevel::ERROR, event); }

void Logger::fatal(LogEvent::ptr event) { log(LogLevel::FATAL, event); }

FileAppender::FileAppender(const std::string &filename) : m_filename(filename) {
  m_filestream.open(m_filename, std::ios::out | std::ios::app);
}

void FileAppender::log(Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) {
  if (level >= m_level) {
    uint64_t now = time(0);
    if (now != m_lastTime) {
      reopen();  // 每秒都reopen一下 避免日志文件不存在
      m_lastTime = now;
    }
    MutexType::Lock lock(m_mutex);
    m_filestream << m_formatter->format(logger, level, event);
  }
}

std::string FileAppender::toYamlString() {
  MutexType::Lock lock(m_mutex);
  YAML::Node node;
  node["type"] = "FileAppender";
  node["file"] = m_filename;
  if (m_level != LogLevel::UNKNOW) {
    node["level"] = LogLevel::toString(m_level);
  }
  if (m_hasFormatter && m_formatter) {
    node["formatter"] = m_formatter->getPattern();
  }
  std::stringstream ss;
  ss << node;
  return ss.str();
}

bool FileAppender::reopen() {
  MutexType::Lock lock(m_mutex);
  if (m_filestream) {
    m_filestream.close();
  }
  m_filestream.open(m_filename, std::ios::app);
  return !!m_filestream;  // !!双向取反 m_filestream不为空则返回true否则返回false
}

void StdoutAppender::log(Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) {  // 此处加override会报错
  if (level >= m_level) {
    MutexType::Lock lock(m_mutex);
    std::cout << m_formatter->format(logger, level, event);
  }
}

std::string StdoutAppender::toYamlString() {
  MutexType::Lock lock(m_mutex);
  YAML::Node node;
  node["type"] = "StdoutAppender";
  if (m_level != LogLevel::UNKNOW) {
    node["level"] = LogLevel::toString(m_level);
  }
  if (m_hasFormatter && m_formatter) {
    node["formatter"] = m_formatter->getPattern();
  }
  std::stringstream ss;
  ss << node;
  return ss.str();
}

LogFormatter::LogFormatter(const std::string &pattern) : m_pattern(pattern) { init(); }

std::string LogFormatter::format(Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) {
  std::stringstream ss;
  for (auto &i : m_items) {
    i->format(ss, logger, level, event);
  }
  return ss.str();
}

// %xxx %xxx{xxx} %%
void LogFormatter::init() {
  // str, format, type
  std::vector<std::tuple<std::string, std::string, int>> vec;
  std::string n_str;
  size_t m_size = m_pattern.size();
  // "%d [%p] <%f:%l>\t%m %n"
  for (size_t i = 0; i < m_size; ++i) {
    if (m_pattern[i] != '%') {  // 解析字母、[]、<>、\t和空格
      n_str.append(1, m_pattern[i]);
      continue;
    }

    size_t n = i + 1;
    if (n < m_size) {  // 解析连续两个%
      if (m_pattern[n] == '%') {
        n_str.append(1, '%');
        continue;
      }
    }

    int fmt_status = 0;
    size_t fmt_begin = 0;

    std::string str;
    std::string fmt;

    while (n < m_size) {  // 解析%后的字母和{}内的内容
      if (!fmt_status && (!isalpha(m_pattern[n]) && m_pattern[n] != '{' && m_pattern[n] != '}')) {
        str = m_pattern.substr(i + 1, n - i - 1);
        break;
      }

      if (fmt_status == 0) {
        if (m_pattern[n] == '{') {
          str = m_pattern.substr(i + 1, n - i - 1);  // 截取%后的字母和{
          fmt_status = 1;
          fmt_begin = n;
          ++n;
          continue;
        }
      } else if (fmt_status == 1) {
        if (m_pattern[n] == '}') {
          fmt = m_pattern.substr(fmt_begin + 1, n - fmt_begin - 1);
          fmt_status = 0;
          ++n;
          break;
        }
      }
      ++n;
      if (n == m_pattern.size()) {
        if (str.empty()) {
          str = m_pattern.substr(i + 1);
        }
      }
    }

    if (fmt_status == 0) {   // %后只有字母
      if (!n_str.empty()) {  // %前有一些其他非字母的字符
        vec.push_back(std::make_tuple(n_str, "", 0));
        n_str.clear();
      }
      vec.push_back(std::make_tuple(str, fmt, 1));  // ('d', "%Y-%m-%d %H:%M:%S", 1) 或 ('T', "", 1)
      i = n - 1;                                    // n - 1 = i + 1
    } else if (fmt_status == 1) {
      std::cout << "pattern parse error " << m_pattern << " - " << m_pattern.substr(i) << std::endl;
      vec.push_back(std::make_tuple("<<pattern_error>>", fmt, 0));
      m_error = true;
    }
  }

  if (!n_str.empty()) {  // 防止最后一个%后的字符丢失
    vec.push_back(std::make_tuple(n_str, "", 0));
  }

  static std::map<std::string, std::function<FormatItem::ptr(const std::string &str)>> s_format_items = {
#define XX(str, C)                                                           \
  {                                                                          \
#str, [](const std::string &fmt) { return FormatItem::ptr(new C(fmt)); } \
  }

    XX(m, MessageFormatItem), XX(p, LevelFormatItem),    XX(c, NameFormatItem),     XX(t, ThreadIdFormatItem),
    XX(n, NewLineFormatItem), XX(d, DateTimeFormatItem), XX(f, FilenameFormatItem), XX(l, LineFormatItem),
    XX(T, TabFormatItem),     XX(F, FiberIdFormatItem)
#undef XX
  };

  for (auto &i : vec) {
    if (std::get<2>(i) == 0) {  // type为0的都是非字母的字符 直接输出
      m_items.push_back(FormatItem::ptr(new StringFormatItem(std::get<0>(i))));
    } else {  // type为1的都是字母字符
      auto it = s_format_items.find(std::get<0>(i));
      if (it == s_format_items.end()) {  // 找不到该字母对应的处理函数
        m_items.push_back(FormatItem::ptr(new StringFormatItem("<<error_format %" + std::get<0>(i) + ">>")));
        m_error = true;
      } else {
        m_items.push_back(it->second(std::get<1>(i)));
      }
    }

    // std::cout << std::get<0>(i) << " - " << std::get<1>(i) << " - " << std::get<2>(i) << std::endl;
  }
}

LogManager::LogManager() {
  m_root.reset(new Logger);
  m_root->addAppender(LogAppender::ptr(new StdoutAppender));
  m_loggers[m_root->m_name] = m_root;

  init();
}

std::string LogManager::toYamlString() {
  MutexType::Lock lock(m_mutex);
  YAML::Node node;
  for (auto &i : m_loggers) {
    node.push_back(YAML::Load(i.second->toYamlString()));
  }
  std::stringstream ss;
  ss << node;
  return ss.str();
}

struct LogAppenderDefine {
  int type = 0;  // 1 FileAppender 2 StdoutAppender
  LogLevel::Level level = LogLevel::Level::UNKNOW;
  std::string file;
  std::string formatter;

  bool operator==(const LogAppenderDefine &oth) const {
    return this->type == oth.type && this->level == oth.level && this->file == oth.file &&
           this->formatter == oth.formatter;
  }
};

struct LogDefine {
  std::string name;
  LogLevel::Level level = LogLevel::Level::UNKNOW;
  std::string formatter;
  std::vector<LogAppenderDefine> appenders;

  bool operator==(const LogDefine &oth) const {
    return this->name == oth.name && this->level == oth.level && this->formatter == oth.formatter &&
           this->appenders == oth.appenders;
  }

  bool operator<(const LogDefine &val) const { return this->name < val.name; }
};

template <>
class LexicalCast<LogAppenderDefine, std::string> {
 public:
  std::string operator()(const LogAppenderDefine &lad) {
    YAML::Node node;
    node["level"] = LogLevel::toString(lad.level);
    if (lad.type == 1) {  // FileAppender
      node["type"] = "FileAppender";
      if (!lad.file.empty()) {
        node["file"] = lad.file;
      }
    } else if (lad.type == 2) {  // StdoutAppender
      node["type"] = "StdoutAppender";
    }

    if (!lad.formatter.empty()) {
      node["formatter"] = lad.formatter;
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
  }
};

template <>
class LexicalCast<std::string, LogAppenderDefine> {
 public:
  LogAppenderDefine operator()(const std::string &str) {
    YAML::Node node = YAML::Load(str);
    LogAppenderDefine lad;
    auto type = node["type"].as<std::string>();
    if (type == "FileAppender") {
      lad.type = 1;
      if (node["file"].IsDefined()) {
        lad.file = node["file"].as<std::string>();
      }
    } else if (type == "StdoutAppender") {
      lad.type = 2;
    }

    if (node["formatter"].IsDefined()) {
      lad.formatter = node["formatter"].as<std::string>();
    }

    if (node["level"].IsDefined()) {
      lad.level = LogLevel::fromString(node["level"].as<std::string>());
    }
    return lad;
  }
};

template <>
class LexicalCast<LogDefine, std::string> {
 public:
  std::string operator()(const LogDefine &ld) {
    YAML::Node node;
    node["name"] = ld.name;
    node["level"] = LogLevel::toString(ld.level);
    if (!ld.formatter.empty()) {
      node["formatter"] = ld.formatter;
    }
    if (!ld.appenders.empty()) {
      node["appenders"] = LexicalCast<std::vector<LogAppenderDefine>, std::string>()(ld.appenders);
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
  }
};

template <>
class LexicalCast<std::string, LogDefine> {
 public:
  LogDefine operator()(const std::string &str) {
    YAML::Node node = YAML::Load(str);
    LogDefine ld;
    ld.name = node["name"].as<std::string>();
    ld.level = LogLevel::fromString(node["level"].as<std::string>());
    if (node["formatter"].IsDefined()) {
      ld.formatter = node["formatter"].as<std::string>();
    }
    if (node["appenders"].IsDefined()) {
      std::stringstream ss;
      ss << node["appenders"];
      ld.appenders = LexicalCast<std::string, std::vector<LogAppenderDefine>>()(ss.str());
    }
    return ld;
  }
};

sylar::ConfigVar<std::set<LogDefine>>::ptr log_set_ptr =
  sylar::Config::Lookup("logs", std::set<LogDefine>{}, "logs config");

// main之前和之后执行东西
// 全局对象在main函数之前初始化 初始化为函数指针
struct LogIniter {
  LogIniter() {
    log_set_ptr->addListener([](const std::set<LogDefine> &old_val, const std::set<LogDefine> &new_val) {
      SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "on_log_config_changed";
      for (auto &i : new_val) {
        auto it = old_val.find(i);
        Logger::ptr logger;
        if (it == old_val.end()) {
          // 新增
          logger = SYLAR_LOG_NAME(i.name);
        } else {
          if (!(i == *it)) {
            // 修改
            logger = SYLAR_LOG_NAME(i.name);
          }
        }
        if (i.level != LogLevel::UNKNOW) {
          logger->setLevel(i.level);
        }
        if (!i.formatter.empty()) {
          logger->setFormatter(i.formatter);
        }

        logger->clearAppenders();
        for (auto &appender : i.appenders) {
          LogAppender::ptr app;
          if (appender.type == 1) {  // FileAppender
            app.reset(new FileAppender(appender.file));
          } else if (appender.type == 2) {  // StdoutAppender
            app.reset(new StdoutAppender());
          }
          app->setLevel(appender.level);
          if (!appender.formatter.empty()) {
            LogFormatter::ptr format(new LogFormatter(appender.formatter));
            if (!format->isError()) {
              app->setFormatter(format);
            } else {
              std::cout << "formatter " << format->getPattern() << " is invalid." << std::endl;
            }
          }
          logger->addAppender(app);
        }
      }

      for (auto &i : old_val) {
        auto it = new_val.find(i);
        if (it == new_val.end()) {
          // 删除
          auto logger = SYLAR_LOG_NAME(i.name);
          logger->setLevel((LogLevel::Level)100);  // 设置一个比较大的Level值 使得当前logger无法使用
          logger->clearAppenders();
        }
      }
    });
  }
};  // namespace sylar

// 全局对象在main函数之前初始化
static LogIniter __log_init;

void LogManager::init() {}

Logger::ptr LogManager::getLogger(const std::string &name) {
  MutexType::Lock lock(m_mutex);
  auto it = m_loggers.find(name);
  if (it != m_loggers.end()) {
    return it->second;
  }

  Logger::ptr logger(new Logger(name));
  logger->m_root = m_root;
  m_loggers[name] = logger;
  return logger;
}
}  // namespace sylar