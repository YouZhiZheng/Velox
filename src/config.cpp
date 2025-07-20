#include "config.hpp"

#include <filesystem>

namespace velox::config
{
  ConfigVarBase::ptr Config::getConfigVarBasePtr(const std::string& name)
  {
    auto it = getDatas().find(name);
    return (it == getDatas().end() ? nullptr : it->second);
  }

  /**
   * @brief 递归地将一个YAML节点扁平化一系列"点分路径"键和对应节点的键值对
   * @details 递归遍历YAML节点, 构造以点号连接的键路径, 并将每个路径及其对应
   *          的 YAML::Node 添加到输出列表中, 从而实现将 YAML 的层级结构扁平化
   * @param[in] prefix 当前键路径的前缀(初始值为空)
   * @param[in] node 当前处理的 YAML 节点
   * @param[out] output 存储扁平化结果
   */
  static void listAllMembers(
      const std::string& prefix,
      const YAML::Node& node,
      std::vector<std::pair<const std::string, const YAML::Node>>& output)
  {
    if (!prefix.empty() && !velox::util::isValidName(prefix))
    {
      VELOX_ERROR("Config invalid name: {} : {}", prefix, YAML::Dump(node));
      return;
    }

    if (!prefix.empty())
    {
      output.emplace_back(prefix, node);
    }

    if (node.IsMap())
    {
      for (const auto& kv : node)
      {
        const std::string key = kv.first.Scalar();  // 获取当前 Map 的键

        // 构造下一层的路径
        std::string new_prefix;
        new_prefix.reserve(prefix.size() + 1 + key.size());
        if (!prefix.empty())
        {
          new_prefix += prefix;
          new_prefix += '.';
        }
        new_prefix += key;

        listAllMembers(new_prefix, kv.second, output);
      }
    }
  }

  void Config::loadFromYaml(const YAML::Node& root)
  {
    std::vector<std::pair<const std::string, const YAML::Node>> all_nodes;
    listAllMembers("", root, all_nodes);

    for (const auto& node_pair : all_nodes)
    {
      const std::string& key = node_pair.first;
      ConfigVarBase::ptr var_ptr = getConfigVarBasePtr(key);

      // 判断该 key 是否已经注册, 注册则更新其值
      if (var_ptr)
      {
        // 在这里 YAML::Node 只会是标量或列表
        if (node_pair.second.IsScalar())
        {
          var_ptr->fromString(node_pair.second.Scalar());
        }
        else
        {
          std::stringstream ss;
          ss << node_pair.second;
          var_ptr->fromString(ss.str());
        }
      }
      else
      {
        VELOX_WARN("Unrecognized config key: {}", key);
      }
    }
  }

  namespace fs = std::filesystem;
  /**
   * @brief 获取指定文件的上次写入时间戳缓存(用于文件变更检测)
   * @param file 配置文件路径
   * @return uint64_t& 可修改的时间戳缓存值
   */
  inline uint64_t& getCachedTimestampForFile(const fs::path& file)
  {
    static std::unordered_map<fs::path, uint64_t> file_last_write_times;
    return file_last_write_times[file];
  }

  void Config::loadFromConfDir(const std::string& relative_dir_path, bool force)
  {
    auto files = velox::util::listAllFilesByExt(relative_dir_path, ".yml");

    for (const auto& file : files)
    {
      std::error_code ec;
      auto last_write_time = fs::last_write_time(file, ec);
      if (ec)
      {
        VELOX_WARN("Skip config file '{}': failed to get last write time: {}", file.string(), ec.message());
        continue;
      }

      auto current_timestamp = velox::util::toUnixTimestamp(last_write_time);
      auto& cached_timestamp = getCachedTimestampForFile(file);

      if (!force && current_timestamp == cached_timestamp)
      {
        VELOX_INFO("Skip config file '{}': unchanged since last load.", file.string());
        continue;
      }

      try
      {
        cached_timestamp = current_timestamp;
        YAML::Node root = YAML::LoadFile(file);
        loadFromYaml(root);
        VELOX_INFO("Loaded config file '{}'", file.string());
      }
      catch (const std::exception& e)
      {
        VELOX_ERROR("Failed to load config file '{}': {}", file.string(), e.what());
      }
      catch (...)
      {
        VELOX_ERROR("Failed to load config file '{}': unknown error.", file.string());
      }
    }
  }
}  // namespace velox::config