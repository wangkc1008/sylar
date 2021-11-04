#include <iostream>

#include "../sylar/config.h"
#include "../sylar/log.h"
#include "yaml-cpp/yaml.h"

#if 0
sylar::ConfigVar<int>::ptr ptr = sylar::Config::Lookup("system.port", 8080, "system port");
sylar::ConfigVar<int>::ptr f_ptr = sylar::Config::Lookup("system.value", (int)10, "system value");
sylar::ConfigVar<std::vector<int>>::ptr vec_int_ptr =
  sylar::Config::Lookup("system.data", std::vector<int>{1, 2}, "system data");
sylar::ConfigVar<std::list<int>>::ptr list_int_ptr =
  sylar::Config::Lookup("system.list", std::list<int>{12, 23}, "system list");
sylar::ConfigVar<std::set<float>>::ptr set_float_ptr =
  sylar::Config::Lookup("system.set", std::set<float>{1.2f, 4.8f, 5.6f}, "system set");
sylar::ConfigVar<std::unordered_set<int>>::ptr u_set_int_ptr =
  sylar::Config::Lookup("system.u_set", std::unordered_set<int>{5, 3, 4}, "system u_set");
sylar::ConfigVar<std::map<std::string, double>>::ptr map_str_dou_ptr =
  sylar::Config::Lookup("system.map", std::map<std::string, double>{{"a", 5.05}, {"b", 6.08}}, "system map");
sylar::ConfigVar<std::unordered_map<std::string, double>>::ptr u_map_str_dou_ptr = sylar::Config::Lookup(
  "system.u_map", std::unordered_map<std::string, double>{{"k", 6.66}, {"v", 7.77}}, "system u_map");

void print_yaml(const YAML::Node &node, int level) {
  if (node.IsScalar()) {
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << std::string(level * 4, ' ') << node.Scalar() << " - " << node.Type() << " - "
                                     << level;
  } else if (node.IsNull()) {
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << std::string(level * 4, ' ') << "NULL - " << node.Type() << " - " << level;
  } else if (node.IsMap()) {  // first是string second是node
    auto it = node.begin();
    for (; it != node.end(); ++it) {
      SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << std::string(level * 4, ' ') << it->first << " - " << it->second.Type()
                                       << " - " << level;
      print_yaml(it->second, level + 1);
    }
  } else if (node.IsSequence()) {
    for (size_t i = 0; i < node.size(); ++i) {
      SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << std::string(level * 4, ' ') << i << " - " << node[i].Type() << " - " << level;
      print_yaml(node[i], level + 1);
    }
  }
}

int test_yaml() {
  YAML::Node root = YAML::LoadFile("/home/wangkc/demo/sylar/bin/conf/log.yaml");
  print_yaml(root, 0);
  // SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << root;
  return 0;
}

void test_config() {
  SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << ptr->getValue();
  SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << f_ptr->getValue();

#define XX(g_var, name, prefix)                                                                    \
  {                                                                                                \
    auto &var = g_var->getValue();                                                                 \
    for (auto &i : var) {                                                                          \
      SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << #prefix << " " << #name << ": " << i;                    \
    }                                                                                              \
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << #prefix << " " << #name << " yaml: " << g_var->toString(); \
  }

#define XX_M(g_var, name, prefix)                                                                          \
  {                                                                                                        \
    auto &var = g_var->getValue();                                                                         \
    for (auto &i : var) {                                                                                  \
      SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << #prefix << " " << #name << ": " << i.first << " : " << i.second; \
    }                                                                                                      \
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << #prefix << " " << #name << " yaml: " << g_var->toString();         \
  }

  XX(vec_int_ptr, int_vec, before);
  XX(list_int_ptr, int_list, before);
  XX(set_float_ptr, float_set, before);
  XX(u_set_int_ptr, int_u_set, before);
  XX_M(map_str_dou_ptr, map_str_dou, before);
  XX_M(u_map_str_dou_ptr, map_str_dou, before);

  YAML::Node root = YAML::LoadFile("/home/wangkc/demo/sylar/bin/conf/log.yaml");
  sylar::Config::LoadFromYaml(root);

  SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << ptr->getValue();
  SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << f_ptr->getValue();

  XX(vec_int_ptr, int_vec, after);
  XX(list_int_ptr, int_list, after);
  XX(set_float_ptr, float_set, after);
  XX(u_set_int_ptr, int_u_set, after);
  XX_M(map_str_dou_ptr, map_str_dou, after);
  XX_M(u_map_str_dou_ptr, map_str_dou, after);
#undef XX
#undef XX_M
}

class Person {
 public:
  std::string toString() const {
    std::stringstream ss;
    ss << "[Person name=" << m_name << ", age=" << m_age << ", sex=" << m_sex << "]";
    return ss.str();
  }

  bool operator==(const Person &p) const {
    return p.m_name == this->m_name && p.m_age == this->m_age && p.m_sex == this->m_sex;
  }

  std::string m_name = "";
  int m_age = 0;
  bool m_sex = false;
};

namespace sylar {
template <>
class LexicalCast<Person, std::string> {
 public:
  std::string operator()(const Person &p) {
    YAML::Node node;
    node["name"] = p.m_name;
    node["age"] = p.m_age;
    node["sex"] = p.m_sex;
    std::stringstream ss;
    ss << node;
    return ss.str();
  }
};

template <>
class LexicalCast<std::string, Person> {
 public:
  Person operator()(const std::string &str) {
    YAML::Node node = YAML::Load(str);
    Person p;
    p.m_name = node["name"].as<std::string>();
    p.m_age = node["age"].as<int>();
    p.m_sex = node["sex"].as<bool>();
    return p;
  }
};
}  // namespace sylar

sylar::ConfigVar<Person>::ptr person_ptr = sylar::Config::Lookup("class.person", Person(), "class person");
sylar::ConfigVar<std::map<std::string, Person>>::ptr map_person_ptr =
  sylar::Config::Lookup("class.map", std::map<std::string, Person>{{"hehe", Person()}}, "class map");
sylar::ConfigVar<std::map<std::string, std::vector<Person>>>::ptr map_vec_person_ptr = sylar::Config::Lookup(
  "class.vec_map", std::map<std::string, std::vector<Person>>{{"p1", std::vector<Person>{Person()}}}, "class vec_map");

void test_class() {
  SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << " before " << person_ptr->getValue().toString() << " - "
                                   << person_ptr->toString();
  person_ptr->addListener([](const Person &old_val, const Person &new_val) {
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "old_val:" << old_val.toString() << " new_val:" << new_val.toString();
  });

#define XX_PM(g_var, prefix)                                                                            \
  {                                                                                                     \
    auto tmp = g_var->getValue();                                                                       \
    for (auto &it : tmp) {                                                                              \
      SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << #prefix << ": " << it.first << " - " << it.second.toString(); \
    }                                                                                                   \
  }

  XX_PM(map_person_ptr, before);
  SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "before: " << map_vec_person_ptr->toString();

  YAML::Node root = YAML::LoadFile("/home/wangkc/demo/sylar/bin/conf/log.yaml");
  sylar::Config::LoadFromYaml(root);

  SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "after " << person_ptr->getValue().toString() << " - " << person_ptr->toString();
  SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "after: " << map_vec_person_ptr->toString();
  XX_PM(map_person_ptr, after);
#undef XX_PM
}
#endif

void test_log() {
  static sylar::Logger::ptr system_ptr = SYLAR_LOG_NAME("system");
  SYLAR_LOG_INFO(system_ptr) << "hello system";
  std::cout << sylar::LoggerMgr::getInstance()->toYamlString() << std::endl;
  YAML::Node root = YAML::LoadFile("/home/wangkc/demo/sylar/bin/conf/log.yaml");
  sylar::Config::LoadFromYaml(root);
  std::cout << "==================" << std::endl;
  std::cout << sylar::LoggerMgr::getInstance()->toYamlString() << std::endl;
  SYLAR_LOG_INFO(system_ptr) << "hello system";

  system_ptr->setFormatter("%d - %m%n");
  std::cout << "==================" << std::endl;
  std::cout << sylar::LoggerMgr::getInstance()->toYamlString() << std::endl;
  SYLAR_LOG_ERROR(system_ptr) << "hello system";
}

int main(int argv, char **argc) {
  // test_yaml();
  // test_config();
  // test_class();
  test_log();
  sylar::Config::Visit([](sylar::ConfigVarBase::ptr var) {
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "name=" << var->getName() << " description=" << var->getDescription()
                                     << " typename" << var->getTypeName() << " value=" << var->toString();
  });
  return 0;
}