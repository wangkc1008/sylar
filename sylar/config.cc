#include "config.h"

#include <utility>

namespace sylar {
ConfigVarBase::ptr Config::LookupBase(const std::string &key) {
  RWMutexType::ReadLock lock(GetRWMutex());
  auto it = GetData().find(key);
  return it == GetData().end() ? nullptr : it->second;
}

static void ListAllNodes(const std::string &prefix, const YAML::Node &node,
                         std::list<std::pair<std::string, const YAML::Node>> &output) {
  if (prefix.find_first_not_of("abcdefghizklmnopqrstuvwxyz._0123456789") != std::string::npos) {
    SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "Config name invalid " << prefix << " : " << node;
    return;
  }

  output.push_back(std::make_pair(prefix, node));
  if (node.IsMap()) {
    auto it = node.begin();
    for (; it != node.end(); ++it) {
      ListAllNodes(prefix.empty() ? it->first.Scalar() : prefix + "." + it->first.Scalar(), it->second, output);
    }
  }
}

void Config::LoadFromYaml(const YAML::Node &root) {
  std::list<std::pair<std::string, const YAML::Node>> all_nodes;
  ListAllNodes("", root, all_nodes);

  for (auto &item : all_nodes) {
    std::string key = item.first;
    if (key.empty()) {
      continue;
    }

    std::transform(key.begin(), key.end(), key.begin(), ::tolower);
    ConfigVarBase::ptr var = LookupBase(key);
    if (var) {
      if (item.second.IsScalar()) {
        std::cout << "IsScalar: " << key << " " << item.second.Scalar() << std::endl;
        var->fromString(item.second.Scalar());
      } else {
        std::stringstream ss;
        ss << item.second;
        // std::cout << "ss: " << key << " " << ss.str() << std::endl;
        var->fromString(ss.str());
      }
    }
  }
}

void Config::Visit(std::function<void(ConfigVarBase::ptr)> cb) {
  RWMutexType::ReadLock lock(GetRWMutex());
  ConfigVarMap& m = GetData();
  for (auto& i : m) {
    cb(i.second);
  }
}
}  // namespace sylar