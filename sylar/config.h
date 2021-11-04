#ifndef __SYLAR_CONFIG_H__
#define __SYLAR_CONFIG_H__

#include "log.h"

#include <algorithm>
#include <boost/lexical_cast.hpp>
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <yaml-cpp/yaml.h>

#include "thread.h"
#include "util.h"

namespace sylar {

class ConfigVarBase {
 public:
  typedef std::shared_ptr<ConfigVarBase> ptr;
  ConfigVarBase(const std::string &name, const std::string &description) : m_name(name), m_description(description) {
    // std::tolower存在两个重载 头文件cctype和头文件locale
    // 默认情况下，在全局名称空间中，tolower 只有一种声明形式 ::tolower
    std::transform(m_name.begin(), m_name.end(), m_name.begin(), ::tolower);
  }
  virtual ~ConfigVarBase() {}  // 虚析构 基类不能只声明必须定义
                               // 避免delete执指向子类的基类指针时只析构父类对象 不释放子类内存的现象

  const std::string &getName() const { return m_name; }
  const std::string &getDescription() const { return m_description; }

  virtual std::string toString() = 0;                   // 纯虚函数 子类必须实现
  virtual bool fromString(const std::string &val) = 0;  // 纯虚函数 子类必须实现
  virtual std::string getTypeName() const = 0;

 protected:
  std::string m_name;
  std::string m_description;
};

template <class F, class T>
class LexicalCast {
 public:
  T operator()(const F &val) { return boost::lexical_cast<T>(val); }
};

template <typename T>
class LexicalCast<std::vector<T>, std::string> {
 public:
  std::string operator()(const std::vector<T> &vec) {
    YAML::Node node;
    for (auto &item : vec) {
      node.push_back(YAML::Load(LexicalCast<T, std::string>()(item)));
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
  }
};

template <typename T>
class LexicalCast<std::string, std::vector<T>> {
 public:
  std::vector<T> operator()(const std::string &str) {
    YAML::Node node = YAML::Load(str);
    typename std::vector<T> vec;
    std::stringstream ss;
    for (size_t i = 0; i < node.size(); ++i) {
      ss.str("");
      ss << node[i];
      vec.push_back(LexicalCast<std::string, T>()(ss.str()));
    }
    return vec;
  }
};

template <typename T>
class LexicalCast<std::list<T>, std::string> {
 public:
  std::string operator()(const std::list<T> &list) {
    YAML::Node node;
    for (auto &item : list) {
      node.push_back(YAML::Load(LexicalCast<T, std::string>()(item)));
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
  }
};

template <typename T>
class LexicalCast<std::string, std::list<T>> {
 public:
  std::list<T> operator()(const std::string &str) {
    YAML::Node node = YAML::Load(str);
    typename std::list<T> list;
    std::stringstream ss;
    for (size_t i = 0; i < node.size(); ++i) {
      ss.str("");
      ss << node[i];
      list.push_back(LexicalCast<std::string, T>()(ss.str()));
    }
    return list;
  }
};

template <typename T>
class LexicalCast<std::set<T>, std::string> {
 public:
  std::string operator()(const std::set<T> &set) {
    YAML::Node node;
    for (auto &item : set) {
      node.push_back(LexicalCast<T, std::string>()(item));
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
  }
};

template <typename T>
class LexicalCast<std::string, std::set<T>> {
 public:
  std::set<T> operator()(const std::string &str) {
    YAML::Node node = YAML::Load(str);
    std::stringstream ss;
    typename std::set<T> set;
    for (size_t i = 0; i < node.size(); ++i) {
      ss.str("");
      ss << node[i];
      set.insert(LexicalCast<std::string, T>()(ss.str()));
    }
    return set;
  }
};

template <typename T>
class LexicalCast<std::unordered_set<T>, std::string> {
 public:
  std::string operator()(const std::unordered_set<T> &u_set) {
    YAML::Node node;
    for (auto &item : u_set) {
      node.push_back(LexicalCast<T, std::string>()(item));
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
  }
};

template <typename T>
class LexicalCast<std::string, std::unordered_set<T>> {
 public:
  std::unordered_set<T> operator()(const std::string &str) {
    YAML::Node node = YAML::Load(str);
    typename std::unordered_set<T> u_set;
    std::stringstream ss;
    for (size_t i = 0; i < node.size(); ++i) {
      ss.str("");
      ss << node[i];
      u_set.insert(LexicalCast<std::string, T>()(ss.str()));
    }
    return u_set;
  }
};

template <typename T>
class LexicalCast<std::map<std::string, T>, std::string> {
 public:
  std::string operator()(const std::map<std::string, T> &map) {
    YAML::Node node;
    for (auto &item : map) {
      node[item.first].push_back(LexicalCast<T, std::string>()(item.second));
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
  }
};

template <typename T>
class LexicalCast<std::string, std::map<std::string, T>> {
 public:
  std::map<std::string, T> operator()(const std::string &str) {
    YAML::Node node = YAML::Load(str);
    typename std::map<std::string, T> map;
    std::stringstream ss;
    for (auto it = node.begin(); it != node.end(); ++it) {
      ss.str("");
      ss << it->second;
      map.insert(std::make_pair(it->first.Scalar(), LexicalCast<std::string, T>()(ss.str())));
    }
    return map;
  }
};

template <typename T>
class LexicalCast<std::unordered_map<std::string, T>, std::string> {
 public:
  std::string operator()(const std::unordered_map<std::string, T> &u_map) {
    YAML::Node node;
    for (auto &item : u_map) {
      node[item.first] = LexicalCast<T, std::string>()(item.second);
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
  }
};

template <typename T>
class LexicalCast<std::string, std::unordered_map<std::string, T>> {
 public:
  std::unordered_map<std::string, T> operator()(const std::string &str) {
    YAML::Node node = YAML::Load(str);
    typename std::unordered_map<std::string, T> u_map;
    std::stringstream ss;
    for (auto it = node.begin(); it != node.end(); ++it) {
      ss.str("");
      ss << it->second;
      u_map.insert(std::make_pair(it->first.Scalar(), LexicalCast<std::string, T>()(ss.str())));
    }
    return u_map;
  }
};

template <class T, class FromStr = LexicalCast<std::string, T>, class ToStr = LexicalCast<T, std::string>>
class ConfigVar : public ConfigVarBase {
 public:
  typedef std::shared_ptr<ConfigVar> ptr;
  typedef std::function<void(const T &old_val, const T &new_val)> on_change_cb;
  typedef RWMutex RWMutexType;

  ConfigVar(const std::string &name, const T &default_val, const std::string &description)
      : ConfigVarBase(name, description), m_val(default_val) {}

  std::string toString() override {
    try {
      RWMutexType::ReadLock lock(m_mutex);
      return ToStr()(m_val);
    } catch (std::exception &e) {
      SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "Config::toString exception" << e.what()
                                        << " convert: " << typeid(m_val).name() << " to string";
    }
    return "";
  }

  bool fromString(const std::string &val) override {
    try {
      setValue(FromStr()(val));
      return true;
    } catch (std::exception &e) {
      SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "Config::fromString exception" << e.what() << " convert: string to "
                                        << typeid(T).name();
    }
    return false;
  }

  const T &getValue() { 
    RWMutexType::ReadLock lock(m_mutex);
    return m_val;
  }

  void setValue(const T &value) {
    {
      RWMutexType::ReadLock lock(m_mutex);
      if (value == m_val) {
        return;
      }
      for (auto &i : m_cbs) {
        i.second(m_val, value);
      }
    }
    RWMutexType::WriteLock lock(m_mutex);
    m_val = value;
  }

  std::string getTypeName() const override { return typeid(T).name(); }

  uint64_t addListener(on_change_cb cb) { 
    static uint64_t s_fun_id = 0;
    RWMutexType::WriteLock lock(m_mutex);
    ++s_fun_id;
    m_cbs[s_fun_id] = cb; 
    return s_fun_id;
  }

  void delListener(uint64_t key) { 
    RWMutexType::WriteLock lock(m_mutex);
    m_cbs.erase(key); 
  }

  void clearListener() { 
    RWMutexType::WriteLock lock(m_mutex);
    m_cbs.clear();
  }

  on_change_cb getListener(uint64_t key) {
    RWMutexType::ReadLock lock(m_mutex);
    auto it = m_cbs.find(key);
    return it == m_cbs.end() ? nullptr : it->secend;
  }

 private:
  T m_val;
  // 变更回调函数组 uint64_t hash key唯一
  std::map<uint64_t, on_change_cb> m_cbs;
  RWMutexType m_mutex;
};

class Config {
 public:
  typedef std::map<std::string, ConfigVarBase::ptr> ConfigVarMap;
  typedef RWMutex RWMutexType;

  template <class T>
  static typename ConfigVar<T>::ptr Lookup(
    const std::string &name,
    const T &default_value,  // typename指出模板声明中的非独立名称是类型名，不是变量名
    const std::string &description = "") {
    RWMutexType::WriteLock lock(GetRWMutex());
    auto it = GetData().find(name);
    if (it != GetData().end()) {
      auto tmp = std::dynamic_pointer_cast<ConfigVar<T>>(it->second);
      if (tmp) {  // 存在Key和类型相同的value
        SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "Lookup name=" << name << " exists.";
        return tmp;
      } else {  // 存在Key但是value的类型不相同
        SYLAR_LOG_ERROR(SYLAR_LOG_ROOT())
          << "Lookup name=" << name << " exists, real type:" << it->second->getTypeName()
          << " real value:" << it->second->toString() << " but got type:" << typeid(T).name();
        return nullptr;
      }
    }

    if (name.find_first_not_of("abcdefghizklmnopqrstuvwxyz._0123456789") !=
        std::string::npos) {  // 不是已上述字符开始的 都属于无效name
      SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "Lookup name invalid" << name;
      throw std::invalid_argument(name);
    }

    typename ConfigVar<T>::ptr v(new ConfigVar<T>(name, default_value, description));
    GetData()[name] = v;
    return v;
  }

  template <class T>
  static typename ConfigVar<T>::ptr Lookup(const std::string &name) {
    RWMutexType::ReadLock lock(GetRWMutex());
    auto it = GetData().find(name);
    if (it == GetData().end()) {
      return nullptr;
    }
    return std::dynamic_pointer_cast<ConfigVar<T>>(it->second);  // 父类指针转为子类指针
  }

  static void LoadFromYaml(const YAML::Node &root);
  static ConfigVarBase::ptr LookupBase(const std::string &key);
  static void Visit(std::function<void(ConfigVarBase::ptr)> cb);
 private:
  // 保证静态变量的初始化顺序 s_data可能初始化顺序晚于调用Lookup的其他对象的顺序 出现s_data未初始化的现象
  // 可以通过函数获取静态对象
  // 把静态对象放到一个返回该对象引用的函数中，函数内的静态对象在函数第一次被调用时进行初始化，且在程序生命周期只被初始化一次
  // 这样静态对象的初始化顺序就是由代码设计而不是链接器的链接顺序来决定的
  static ConfigVarMap& GetData() {
    static ConfigVarMap s_data;
    return s_data;
  }

  static RWMutexType& GetRWMutex() {
    static RWMutexType s_mutex;
    return s_mutex;
  }
};

}  // namespace sylar

#endif  // __SYLAR_CONFIG_H__
