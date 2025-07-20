/**
 * @file config_test.cpp
 * @brief 配置模块测试
 * @author zhengyouzhi
 * @email youzhizheng9@gmail.com
 * @date 2025-7-9
 * @copyright Copyright (c) 2025 zhengyouzhi
 * @license   GNU General Public License v3.0
 *            See LICENSE file in the project root for full license information.
 */

#include "config.hpp"

#include <gtest/gtest.h>

#include <stdexcept>
#include <vector>

#include "log.hpp"

using namespace velox::config;

// 测试 std::vector<T> 的转换
TEST(TypeConverterTest, vector)
{
  TypeConverter<std::vector<int>, std::string> to_string;
  std::vector<int> v = { 10, 20, 30 };
  std::string yaml_str = to_string(v);

  // 验证 YAML 节点
  YAML::Node node = YAML::Load(yaml_str);
  ASSERT_TRUE(node.IsSequence());
  ASSERT_EQ(node.size(), 3);
  EXPECT_EQ(node[0].as<int>(), 10);
  EXPECT_EQ(node[1].as<int>(), 20);
  EXPECT_EQ(node[2].as<int>(), 30);

  // 验证反向转换
  TypeConverter<std::string, std::vector<int>> from_string;
  std::vector<int> v2 = from_string(yaml_str);
  EXPECT_EQ(v, v2);
}

// 测试 std::list<T> 的转换
TEST(TypeConverterTest, list)
{
  TypeConverter<std::list<std::string>, std::string> to_string;
  std::list<std::string> l = { "apple", "banana", "cherry" };
  std::string yaml_str = to_string(l);

  // 验证 YAML 节点
  YAML::Node node = YAML::Load(yaml_str);
  ASSERT_TRUE(node.IsSequence());
  ASSERT_EQ(node.size(), 3);
  EXPECT_EQ(node[0].as<std::string>(), "apple");
  EXPECT_EQ(node[2].as<std::string>(), "cherry");

  // 验证反向转换
  TypeConverter<std::string, std::list<std::string>> from_string;
  std::list<std::string> l2 = from_string(yaml_str);
  EXPECT_EQ(l, l2);
}

// 测试 std::set<T> 的转换
TEST(TypeConverterTest, set)
{
  TypeConverter<std::set<int>, std::string> to_string;
  std::set<int> s = { 100, 1, 50 };  // set 会自动排序为 {1, 50, 100}
  std::string yaml_str = to_string(s);

  // 验证 YAML 节点
  YAML::Node node = YAML::Load(yaml_str);
  ASSERT_TRUE(node.IsSequence());
  ASSERT_EQ(node.size(), 3);
  // 验证时应按照 set 的排序结果来
  EXPECT_EQ(node[0].as<int>(), 1);
  EXPECT_EQ(node[1].as<int>(), 50);
  EXPECT_EQ(node[2].as<int>(), 100);

  // 验证反向转换
  TypeConverter<std::string, std::set<int>> from_string;
  std::set<int> s2 = from_string(yaml_str);
  EXPECT_EQ(s, s2);
}

// 测试 std::unordered_set<T> 的转换
TEST(TypeConverterTest, unorderedSet)
{
  TypeConverter<std::unordered_set<std::string>, std::string> to_string;
  std::unordered_set<std::string> us = { "user", "admin", "guest" };
  std::string yaml_str = to_string(us);

  // 验证 YAML 节点
  YAML::Node node = YAML::Load(yaml_str);
  ASSERT_TRUE(node.IsSequence());
  ASSERT_EQ(node.size(), 3);
  TypeConverter<std::string, std::unordered_set<std::string>> from_string;
  std::unordered_set<std::string> us2 = from_string(yaml_str);
  EXPECT_EQ(us, us2);
}

// 测试 std::map<T> 的转换
TEST(TypeConverterTest, map)
{
  TypeConverter<std::map<std::string, int>, std::string> to_string;
  std::map<std::string, int> m = { { "port", 8080 }, { "timeout", 3000 }, { "retries", 3 } };
  std::string yaml_str = to_string(m);

  // 验证 YAML 节点
  YAML::Node node = YAML::Load(yaml_str);
  ASSERT_TRUE(node.IsMap());
  ASSERT_EQ(node.size(), 3);
  EXPECT_EQ(node["port"].as<int>(), 8080);
  EXPECT_EQ(node["timeout"].as<int>(), 3000);
  EXPECT_EQ(node["retries"].as<int>(), 3);

  // 验证反向转换
  TypeConverter<std::string, std::map<std::string, int>> from_string;
  std::map<std::string, int> m2 = from_string(yaml_str);
  EXPECT_EQ(m, m2);
}

// 测试 std::unordered_map<T> 的转换
TEST(TypeConverterTest, unorderedMap)
{
  TypeConverter<std::unordered_map<std::string, std::string>, std::string> to_string;
  std::unordered_map<std::string, std::string> um = { { "user", "test_user" }, { "token", "abc-123" } };
  std::string yaml_str = to_string(um);

  // 验证 YAML 节点
  YAML::Node node = YAML::Load(yaml_str);
  ASSERT_TRUE(node.IsMap());
  ASSERT_EQ(node.size(), 2);
  EXPECT_EQ(node["user"].as<std::string>(), "test_user");
  EXPECT_EQ(node["token"].as<std::string>(), "abc-123");

  // 验证反向转换
  TypeConverter<std::string, std::unordered_map<std::string, std::string>> from_string;
  std::unordered_map<std::string, std::string> um2 = from_string(yaml_str);
  EXPECT_EQ(um, um2);
}

// 测试 YAML 中最常用的嵌套结构(map 中包含 vector)
TEST(TypeConverterTest, nestedMapWithVector)
{
  using NestedType = std::map<std::string, std::vector<int>>;
  TypeConverter<NestedType, std::string> to_string;

  NestedType m = { { "primary_ports", { 80, 443 } }, { "secondary_ports", { 8080, 8443, 9000 } } };
  std::string yaml_str = to_string(m);

  // 验证 YAML 节点
  YAML::Node node = YAML::Load(yaml_str);
  ASSERT_TRUE(node.IsMap());
  ASSERT_EQ(node.size(), 2);
  ASSERT_TRUE(node["primary_ports"].IsSequence());
  EXPECT_EQ(node["primary_ports"].size(), 2);
  EXPECT_EQ(node["primary_ports"][0].as<int>(), 80);
  ASSERT_TRUE(node["secondary_ports"].IsSequence());
  EXPECT_EQ(node["secondary_ports"].size(), 3);
  EXPECT_EQ(node["secondary_ports"][1].as<int>(), 8443);

  // 验证反向转换
  TypeConverter<std::string, NestedType> from_string;
  NestedType m2 = from_string(yaml_str);
  EXPECT_EQ(m, m2);
}

// 测试自定义类型的转换
// 定义一个类型
namespace velox::test
{
  // 日志输出器类型(强类型枚举)
  enum class AppenderType
  {
    File = 1,
    Stdout,
  };

  struct LogAppenderDefine
  {
    AppenderType type = AppenderType::File;
    std::string level = "DEBUG";
    std::string formatter = "%^[%Y-%m-%d %T.%e][thread %t][%l][%n][%s:%#]: %v%$";
    std::string file;

    bool operator==(const LogAppenderDefine& oth) const
    {
      return type == oth.type && level == oth.level && formatter == oth.formatter && file == oth.file;
    }
  };

  struct LogDefine
  {
    std::string name;
    std::string level = "DEBUG";
    std::string formatter = "%^[%Y-%m-%d %T.%e][thread %t][%l][%n][%s:%#]: %v%$";
    std::vector<LogAppenderDefine> appenders;

    bool operator==(const LogDefine& oth) const
    {
      return name == oth.name && level == oth.level && formatter == oth.formatter && appenders == oth.appenders;
    }
  };
}  // namespace velox::test

// 实现对应的偏特化
namespace velox::config
{
  // string -> LogDefine
  template<>
  class TypeConverter<std::string, velox::test::LogDefine>
  {
   public:
    velox::test::LogDefine operator()(const std::string& v)
    {
      YAML::Node node = YAML::Load(v);
      velox::test::LogDefine log_def;

      if (node["name"].IsDefined())
      {
        log_def.name = node["name"].as<std::string>();
      }
      else
      {
        std::cout << "log config error: name is null\n";
        throw std::logic_error("log config name is null");
      }

      if (node["level"].IsDefined())
      {
        log_def.level = node["level"].as<std::string>();
      }

      if (node["formatter"].IsDefined())
      {
        log_def.formatter = node["formatter"].as<std::string>();
      }

      if (node["appenders"].IsDefined())
      {
        for (size_t i = 0; i < node["appenders"].size(); ++i)
        {
          auto appender = node["appenders"][i];

          if (!appender["type"].IsDefined())
          {
            std::cout << "log config error: appender type is null, " << YAML::Dump(appender) << std::endl;
            continue;
          }

          auto type = appender["type"].as<std::string>();
          velox::test::LogAppenderDefine log_app_def;
          if (type == "FileLogAppender")
          {
            log_app_def.type = velox::test::AppenderType::File;

            if (!appender["file"].IsDefined())
            {
              std::cout << "log config error: fileappender file is null, " << YAML::Dump(appender) << std::endl;
              continue;
            }

            log_app_def.file = appender["file"].as<std::string>();
          }
          else if (type == "StdoutLogAppender")
          {
            log_app_def.type = velox::test::AppenderType::Stdout;
          }
          else
          {
            std::cout << "log config error: appender type is invalid, " << YAML::Dump(appender) << std::endl;
            continue;
          }

          if (appender["level"].IsDefined())
          {
            log_app_def.level = appender["level"].as<std::string>();
          }

          if (appender["formatter"].IsDefined())
          {
            log_app_def.formatter = appender["formatter"].as<std::string>();
          }

          log_def.appenders.push_back(log_app_def);
        }
      }

      return log_def;
    }
  };

  // LogDefine -> string
  template<>
  class TypeConverter<velox::test::LogDefine, std::string>
  {
   public:
    std::string operator()(const velox::test::LogDefine& ld)
    {
      YAML::Node node;
      node["name"] = ld.name;
      node["level"] = ld.level;
      node["formatter"] = ld.formatter;

      for (const auto& app : ld.appenders)
      {
        YAML::Node app_node;

        if (app.type == velox::test::AppenderType::File)
        {
          app_node["type"] = "FileLogAppender";
          app_node["file"] = app.file;
        }
        else
        {
          app_node["type"] = "StdoutLogAppender";
        }

        app_node["level"] = app.level;
        app_node["formatter"] = app.formatter;

        node["appenders"].push_back(app_node);
      }

      std::stringstream ss;
      ss << node;
      return ss.str();
    }
  };

}  // namespace velox::config

TEST(TypeConverterTest, logConfig)
{
  std::string log_yaml = R"(
logs:
  - name: root
    level: info
    appenders:
    - type: FileLogAppender
      file: /apps/logs/sylar/root.txt
    - type: StdoutLogAppender
  - name: system
    level: info
    appenders:
    - type: FileLogAppender
      file: /apps/logs/sylar/system.txt
    - type: StdoutLogAppender)";

  YAML::Node log_node = YAML::Load(log_yaml);
  ASSERT_TRUE(log_node["logs"].IsSequence());

  auto logs = log_node["logs"];
  ASSERT_EQ(logs.size(), 2);

  // 遍历每一个 log 项，并转换为 LogDefine
  for (size_t i = 0; i < logs.size(); ++i)
  {
    YAML::Node log = logs[i];
    std::string single_yaml = YAML::Dump(log);
    velox::test::LogDefine log_def = TypeConverter<std::string, velox::test::LogDefine>()(single_yaml);

    if (i == 0)
    {
      EXPECT_EQ(log_def.name, "root");
      EXPECT_EQ(log_def.level, "info");
      ASSERT_EQ(log_def.appenders.size(), 2);
      EXPECT_EQ(log_def.appenders[0].type, velox::test::AppenderType::File);
      EXPECT_EQ(log_def.appenders[0].file, "/apps/logs/sylar/root.txt");
      EXPECT_EQ(log_def.appenders[1].type, velox::test::AppenderType::Stdout);
    }
    else
    {
      EXPECT_EQ(log_def.name, "system");
      EXPECT_EQ(log_def.level, "info");
      ASSERT_EQ(log_def.appenders.size(), 2);
      EXPECT_EQ(log_def.appenders[0].type, velox::test::AppenderType::File);
      EXPECT_EQ(log_def.appenders[0].file, "/apps/logs/sylar/system.txt");
      EXPECT_EQ(log_def.appenders[1].type, velox::test::AppenderType::Stdout);
    }

    // 反向转换测试(LogDefine -> string)
    std::string encoded = TypeConverter<velox::test::LogDefine, std::string>()(log_def);
    velox::test::LogDefine decoded = TypeConverter<std::string, velox::test::LogDefine>()(encoded);
    EXPECT_EQ(decoded, log_def);
  }
}

class ConfigVarTest : public ::testing::Test
{
 protected:
  void SetUp() override
  {
    bool init_result = VELOX_LOG_INIT();  // 初始化日志
    ASSERT_TRUE(init_result) << "Failed to initialize logger!";
  }

  void TearDown() override { VELOX_LOG_SHUTDOWN(); }
};

// 测试基本类型的 ConfigVar
TEST_F(ConfigVarTest, basicType)
{
  auto var = std::make_shared<ConfigVar<int>>("system.port", 8080, "System Port");

  EXPECT_EQ(var->getName(), "system.port");
  EXPECT_EQ(var->getDescription(), "System Port");
  EXPECT_EQ(var->getTypeName(), "int");
  EXPECT_EQ(var->getValue(), 8080);
  EXPECT_EQ(var->toString(), "8080");

  ASSERT_TRUE(var->fromString("9090"));
  EXPECT_EQ(var->getValue(), 9090);
}

// 测试复杂类型的 ConfigVar
TEST_F(ConfigVarTest, complexType)
{
  std::vector<std::string> default_vec = { "admin", "user" };
  auto var = std::make_shared<ConfigVar<std::vector<std::string>>>("system.users", default_vec, "System Users");

  EXPECT_EQ(var->getName(), "system.users");
  EXPECT_EQ(var->getDescription(), "System Users");
  EXPECT_NE(var->getTypeName(), "");
  EXPECT_EQ(var->getValue(), default_vec);

  std::string yaml_str = var->toString();
  // 预期输出:
  // - admin
  // - user
  // 这里我们用 YAML::Load 来验证正确性
  YAML::Node node = YAML::Load(yaml_str);
  ASSERT_TRUE(node.IsSequence());
  ASSERT_EQ(node.size(), 2);
  EXPECT_EQ(node[0].as<std::string>(), "admin");
  EXPECT_EQ(node[1].as<std::string>(), "user");

  std::string new_yaml = "[guest, root]";
  var->fromString(new_yaml);

  std::vector<std::string> expected_vec = { "guest", "root" };
  EXPECT_EQ(var->getValue(), expected_vec);
}

// 测试 ConfigVar 的变更回调函数
TEST_F(ConfigVarTest, changCallbacks)
{
  ConfigVar<int>::ptr var = std::make_shared<ConfigVar<int>>("test.int", 10, "test integer var");

  // 测试单个回调
  {
    // 定义一组变量, 用于检查回调是否被调用
    int callback_count = 0;
    int old_value = 0;
    int new_value = 0;

    // 注册变更监听器
    uint64_t id = var->addListener(
        [&](const int& old_val, const int& new_val)
        {
          ++callback_count;
          old_value = old_val;
          new_value = new_val;
        });

    // 改变值, 应该触发回调
    var->setValue(20);
    EXPECT_EQ(callback_count, 1);
    EXPECT_EQ(old_value, 10);
    EXPECT_EQ(new_value, 20);

    // 值未变, 不触发回调
    var->setValue(20);
    EXPECT_EQ(callback_count, 1);

    // 删除回调
    var->delListener(id);
    var->setValue(30);
    EXPECT_EQ(callback_count, 1);
  }

  // 测试一组回调
  {
    ConfigVar<std::string>::ptr var = std::make_shared<ConfigVar<std::string>>("test.str", "hello");

    int count1 = 0;
    int count2 = 0;
    int count3 = 0;

    uint64_t id1 = var->addListener([&count1](const std::string& old_val, const std::string& new_val) { ++count1; });
    uint64_t id2 = var->addListener([&count2](const std::string& old_val, const std::string& new_val) { ++count2; });
    uint64_t id3 = var->addListener([&count3](const std::string& old_val, const std::string& new_val) { ++count3; });

    var->setValue("world");
    EXPECT_EQ(count1, 1);
    EXPECT_EQ(count2, 1);
    EXPECT_EQ(count3, 1);

    // 删除 key 为 id1 的回调函数
    var->delListener(id1);
    var->setValue("hello world");
    EXPECT_EQ(count1, 1);
    EXPECT_EQ(count2, 2);
    EXPECT_EQ(count3, 2);

    // 直接调用 key 为 id2 的回调函数
    auto cb = var->getListener(id2);
    ASSERT_NE(cb, nullptr);
    cb("1", "2");
    EXPECT_EQ(count2, 3);

    // 清除全部回调函数
    var->clearAllListener();
    var->setValue("hello world!!!");
    EXPECT_EQ(count1, 1);
    EXPECT_EQ(count2, 3);
    EXPECT_EQ(count3, 2);
    EXPECT_EQ(var->getListener(id1), nullptr);
    EXPECT_EQ(var->getListener(id2), nullptr);
    EXPECT_EQ(var->getListener(id3), nullptr);
  }
}

class ConfigTest : public ::testing::Test
{
 protected:
  void SetUp() override
  {
    bool init_result = VELOX_LOG_INIT();
    ASSERT_TRUE(init_result) << "Failed to initialize logger!";
  }

  void TearDown() override
  {
    Config::clearAllData();  // 清理静态数据，确保测试之间的独立性
    VELOX_LOG_SHUTDOWN();
  }
};

TEST_F(ConfigTest, getOrCreateConfigVarPtr)
{
  // 1. 测试创建功能
  auto port_var = Config::getOrCreateConfigVarPtr<int>("server.port", 8000, "Server Port");
  ASSERT_NE(port_var, nullptr);
  EXPECT_EQ(port_var->getName(), "server.port");
  EXPECT_EQ(port_var->getDescription(), "Server Port");
  EXPECT_EQ(port_var->getValue(), 8000);

  // 2. 再次获取, 应返回同一个实例
  auto port_var_2 = Config::getOrCreateConfigVarPtr<int>("server.port", 9999, "ignored");
  ASSERT_NE(port_var_2, nullptr);
  EXPECT_EQ(port_var.get(), port_var_2.get());  // 应该是同一个智能指针指向的对象
  EXPECT_EQ(port_var_2->getValue(), 8000);      // 传递的9999默认值应被忽略
  EXPECT_EQ(port_var_2->getName(), "server.port");
  EXPECT_EQ(port_var_2->getDescription(), "Server Port");

  // 3. 类型不匹配，应返回 nullptr
  auto port_var_str = Config::getOrCreateConfigVarPtr<std::string>("server.port", "8000");
  EXPECT_EQ(port_var_str, nullptr);

  // 4.测试复杂一点的情况
  std::vector<std::string> temp = { "localhost" };
  auto hosts_var = Config::getOrCreateConfigVarPtr<std::vector<std::string>>("server.hosts", temp);
  ASSERT_NE(hosts_var, nullptr);
  EXPECT_EQ(hosts_var->getValue(), temp);
  auto hosts_var_int_vec = Config::getOrCreateConfigVarPtr<std::vector<int>>("server.hosts", { 127 });
  EXPECT_EQ(hosts_var_int_vec, nullptr);

  // 5. 非法名称
  EXPECT_THROW(Config::getOrCreateConfigVarPtr<int>("invalid-name", 1), std::invalid_argument);
  EXPECT_THROW(Config::getOrCreateConfigVarPtr<int>("InvalidName", 1), std::invalid_argument);
  EXPECT_THROW(Config::getOrCreateConfigVarPtr<std::string>("invalid@name", "value"), std::invalid_argument);
  EXPECT_THROW(Config::getOrCreateConfigVarPtr<std::string>("invalid#name", "value"), std::invalid_argument);
  EXPECT_THROW(Config::getOrCreateConfigVarPtr<std::string>("#invalid.name", "value"), std::invalid_argument);
  EXPECT_THROW(Config::getOrCreateConfigVarPtr<int>("", 100), std::invalid_argument);
}

TEST_F(ConfigTest, getConfigVarPtr)
{
  // 先创建对应配置
  auto tmp = Config::getOrCreateConfigVarPtr<double>("math.pi", 3.14);
  ASSERT_NE(tmp, nullptr);
  EXPECT_EQ(tmp->getName(), "math.pi");
  EXPECT_EQ(tmp->getDescription(), "");
  EXPECT_DOUBLE_EQ(tmp->getValue(), 3.14);

  // 名称, 类型一致
  auto pi_var = Config::getConfigVarPtr<double>("math.pi");
  ASSERT_NE(pi_var, nullptr);
  EXPECT_EQ(pi_var->getName(), "math.pi");
  EXPECT_EQ(pi_var->getDescription(), "");
  EXPECT_DOUBLE_EQ(pi_var->getValue(), 3.14);

  // 类型不一致
  auto pi_var_int = Config::getConfigVarPtr<int>("math.pi");
  EXPECT_EQ(pi_var_int, nullptr);

  // 不存在的配置
  auto non_exist_var = Config::getConfigVarPtr<int>("non.exist.var");
  EXPECT_EQ(non_exist_var, nullptr);
}

// 定义好对应的结构体和偏特化
namespace velox::test
{
  struct ServerDefine
  {
    std::vector<std::string> address;
    int keepalive = 0;  // 可选, 默认 0
    int timeout = 1000;
    std::string name;
    std::string accept_worker;
    std::string io_worker;
    std::string process_worker;
    std::string type;

    bool operator==(const ServerDefine& other) const
    {
      return address == other.address && keepalive == other.keepalive && timeout == other.timeout && name == other.name &&
             accept_worker == other.accept_worker && io_worker == other.io_worker &&
             process_worker == other.process_worker && type == other.type;
    }
  };
}  // namespace velox::test

namespace velox::config
{
  // string -> ServerDefine
  template<>
  class TypeConverter<std::string, velox::test::ServerDefine>
  {
   public:
    velox::test::ServerDefine operator()(const std::string& v)
    {
      YAML::Node node = YAML::Load(v);
      velox::test::ServerDefine def;

      if (node["address"].IsDefined())
      {
        def.address = node["address"].as<std::vector<std::string>>();
      }
      else
      {
        VELOX_ERROR("server address list is null");
        throw std::logic_error("server address list is null");
      }

      if (node["keepalive"].IsDefined())
      {
        def.keepalive = node["keepalive"].as<int>();
      }

      if (node["timeout"].IsDefined())
      {
        def.timeout = node["timeout"].as<int>();
      }

      if (node["name"].IsDefined())
      {
        def.name = node["name"].as<std::string>();
      }
      else
      {
        VELOX_ERROR("server name is empty");
        throw std::logic_error("server name is empty");
      }

      if (node["accept_worker"].IsDefined())
      {
        def.accept_worker = node["accept_worker"].as<std::string>();
      }
      else
      {
        VELOX_ERROR("server accept worker is empty");
        throw std::logic_error("server accept worker is empty");
      }

      if (node["io_worker"].IsDefined())
      {
        def.io_worker = node["io_worker"].as<std::string>();
      }
      else
      {
        VELOX_ERROR("server io worker is empty");
        throw std::logic_error("server io worker is empty");
      }

      if (node["process_worker"].IsDefined())
      {
        def.process_worker = node["process_worker"].as<std::string>();
      }
      else
      {
        VELOX_ERROR("server process worker is empty");
        throw std::logic_error("server process worker is empty");
      }

      if (node["type"].IsDefined())
      {
        def.type = node["type"].as<std::string>();
      }
      else
      {
        VELOX_ERROR("server type is empty");
        throw std::logic_error("server type is empty");
      }

      return def;
    }
  };

  // ServerConfig -> string
  template<>
  class TypeConverter<velox::test::ServerDefine, std::string>
  {
   public:
    std::string operator()(const velox::test::ServerDefine& def)
    {
      YAML::Node node;

      node["address"] = def.address;
      if (def.keepalive != 0)
      {
        node["keepalive"] = def.keepalive;
      }
      node["timeout"] = def.timeout;
      node["name"] = def.name;
      node["accept_worker"] = def.accept_worker;
      node["io_worker"] = def.io_worker;
      node["process_worker"] = def.process_worker;
      node["type"] = def.type;

      std::stringstream ss;
      ss << node;
      return ss.str();
    }
  };

}  // namespace velox::config

TEST_F(ConfigTest, loadFromYaml)
{
  // 注册配置变量
  auto servers_config = Config::getOrCreateConfigVarPtr<std::vector<velox::test::ServerDefine>>(
      "servers", std::vector<velox::test::ServerDefine>(), "servers config");

  // 构造 YAML 内容
  std::string yaml_content = R"(
servers:
  - address: ["0.0.0.0:8090", "127.0.0.1:8091", "/tmp/test.sock"]
    keepalive: 1
    timeout: 1000
    name: sylar/1.1
    accept_worker: accept
    io_worker: http_io
    process_worker: http_io
    type: http
  - address: ["0.0.0.0:8062", "0.0.0.0:8061"]
    timeout: 1000
    name: sylar-rock/1.0
    accept_worker: accept
    io_worker: io
    process_worker: io
    type: rock)";

  // 解析 YAML
  YAML::Node root = YAML::Load(yaml_content);
  Config::loadFromYaml(root);

  // 判断是否正确解析
  auto servers = servers_config->getValue();
  ASSERT_EQ(servers.size(), 2);

  // 检查第一个 serverDefine
  {
    auto server = servers[0];
    std::vector<std::string> address = { "0.0.0.0:8090", "127.0.0.1:8091", "/tmp/test.sock" };
    EXPECT_EQ(server.address, address);
    EXPECT_EQ(server.keepalive, 1);
    EXPECT_EQ(server.timeout, 1000);
    EXPECT_EQ(server.name, "sylar/1.1");
    EXPECT_EQ(server.accept_worker, "accept");
    EXPECT_EQ(server.io_worker, "http_io");
    EXPECT_EQ(server.process_worker, "http_io");
    EXPECT_EQ(server.type, "http");
  }

  // 检查第二个 serverDefine
  {
    auto server = servers[1];
    std::vector<std::string> address = { "0.0.0.0:8062", "0.0.0.0:8061" };
    EXPECT_EQ(server.address, address);
    EXPECT_EQ(server.keepalive, 0);
    EXPECT_EQ(server.timeout, 1000);
    EXPECT_EQ(server.name, "sylar-rock/1.0");
    EXPECT_EQ(server.accept_worker, "accept");
    EXPECT_EQ(server.io_worker, "io");
    EXPECT_EQ(server.process_worker, "io");
    EXPECT_EQ(server.type, "rock");
  }
}

TEST_F(ConfigTest, loadFromConfDir)
{
  /* ----------------------- 注册 worker.yml 配置  -----------------------*/
  auto io_threads = Config::getOrCreateConfigVarPtr<int>("workers.io.thread_num", 0);
  auto http_io_threads = Config::getOrCreateConfigVarPtr<int>("workers.http_io.thread_num", 0);
  auto accept_threads = Config::getOrCreateConfigVarPtr<int>("workers.accept.thread_num", 0);
  auto worker_threads = Config::getOrCreateConfigVarPtr<int>("workers.worker.thread_num", 0);
  auto notify_threads = Config::getOrCreateConfigVarPtr<int>("workers.notify.thread_num", 0);
  auto service_io_threads = Config::getOrCreateConfigVarPtr<int>("workers.service_io.thread_num", 0);

  /* ----------------------- 注册 server.yml 配置  -----------------------*/
  auto servers_config = Config::getOrCreateConfigVarPtr<std::vector<velox::test::ServerDefine>>(
      "servers", std::vector<velox::test::ServerDefine>(), "servers config");

  /* ----------------------- 注册 log.yml 配置  -----------------------*/
  auto logs_config = Config::getOrCreateConfigVarPtr<std::vector<velox::test::LogDefine>>(
      "logs", std::vector<velox::test::LogDefine>(), "logs config");

  // 加载指定目录下的配置文件
  Config::loadFromConfDir("test/config");

  // 验证 log.yml是否正确加载
  {
    auto logs = logs_config->getValue();
    ASSERT_EQ(logs.size(), 2);

    // 检查第一个 logDefine
    {
      auto log = logs[0];
      EXPECT_EQ(log.name, "root");
      EXPECT_EQ(log.level, "info");
      ASSERT_EQ(log.appenders.size(), 2);
      EXPECT_EQ(log.appenders[0].type, velox::test::AppenderType::File);
      EXPECT_EQ(log.appenders[0].file, "/apps/logs/sylar/root.txt");
      EXPECT_EQ(log.appenders[1].type, velox::test::AppenderType::Stdout);
    }

    // 检查第二个 logDefine
    {
      auto log = logs[1];
      EXPECT_EQ(log.name, "system");
      EXPECT_EQ(log.level, "info");
      ASSERT_EQ(log.appenders.size(), 2);
      EXPECT_EQ(log.appenders[0].type, velox::test::AppenderType::File);
      EXPECT_EQ(log.appenders[0].file, "/apps/logs/sylar/system.txt");
      EXPECT_EQ(log.appenders[1].type, velox::test::AppenderType::Stdout);
    }
  }

  // 验证 server.yml是否正确加载
  {
    auto servers = servers_config->getValue();
    ASSERT_EQ(servers.size(), 2);

    // 检查第一个 serverDefine
    {
      auto server = servers[0];
      std::vector<std::string> address = { "0.0.0.0:8090", "127.0.0.1:8091", "/tmp/test.sock" };
      EXPECT_EQ(server.address, address);
      EXPECT_EQ(server.keepalive, 1);
      EXPECT_EQ(server.timeout, 1000);
      EXPECT_EQ(server.name, "sylar/1.1");
      EXPECT_EQ(server.accept_worker, "accept");
      EXPECT_EQ(server.io_worker, "http_io");
      EXPECT_EQ(server.process_worker, "http_io");
      EXPECT_EQ(server.type, "http");
    }

    // 检查第二个 serverDefine
    {
      auto server = servers[1];
      std::vector<std::string> address = { "0.0.0.0:8062", "0.0.0.0:8061" };
      EXPECT_EQ(server.address, address);
      EXPECT_EQ(server.keepalive, 0);
      EXPECT_EQ(server.timeout, 1000);
      EXPECT_EQ(server.name, "sylar-rock/1.0");
      EXPECT_EQ(server.accept_worker, "accept");
      EXPECT_EQ(server.io_worker, "io");
      EXPECT_EQ(server.process_worker, "io");
      EXPECT_EQ(server.type, "rock");
    }
  }

  // 验证 worker.yml是否正确加载
  {
    EXPECT_EQ(io_threads->getValue(), 8);
    EXPECT_EQ(http_io_threads->getValue(), 1);
    EXPECT_EQ(accept_threads->getValue(), 2);
    EXPECT_EQ(worker_threads->getValue(), 8);
    EXPECT_EQ(notify_threads->getValue(), 8);
    EXPECT_EQ(service_io_threads->getValue(), 4);
  }
}

TEST_F(ConfigTest, loadFromConfDirTimestamp)
{
  // 注册 worker.yml 配置
  auto io_threads = Config::getOrCreateConfigVarPtr<int>("workers.io.thread_num", 0);
  auto http_io_threads = Config::getOrCreateConfigVarPtr<int>("workers.http_io.thread_num", 0);
  auto accept_threads = Config::getOrCreateConfigVarPtr<int>("workers.accept.thread_num", 0);
  auto worker_threads = Config::getOrCreateConfigVarPtr<int>("workers.worker.thread_num", 0);
  auto notify_threads = Config::getOrCreateConfigVarPtr<int>("workers.notify.thread_num", 0);
  auto service_io_threads = Config::getOrCreateConfigVarPtr<int>("workers.service_io.thread_num", 0);

  // 第一次加载, 强制加载, 因为前面的 loadFromConfDir 已经缓存文件时间戳了
  Config::loadFromConfDir("test/config", true);
  EXPECT_EQ(io_threads->getValue(), 8);
  EXPECT_EQ(http_io_threads->getValue(), 1);
  EXPECT_EQ(accept_threads->getValue(), 2);
  EXPECT_EQ(worker_threads->getValue(), 8);
  EXPECT_EQ(notify_threads->getValue(), 8);
  EXPECT_EQ(service_io_threads->getValue(), 4);

  // 修改配置参数值
  io_threads->setValue(1);
  http_io_threads->setValue(2);
  accept_threads->setValue(3);

  // 第二次加载, 文件未改动, 参数值应该不变
  Config::loadFromConfDir("test/config");
  EXPECT_EQ(io_threads->getValue(), 1);
  EXPECT_EQ(http_io_threads->getValue(), 2);
  EXPECT_EQ(accept_threads->getValue(), 3);

  // 第三次加载, 强制加载, 参数值应该恢复
  Config::loadFromConfDir("test/config", true);
  EXPECT_EQ(io_threads->getValue(), 8);
  EXPECT_EQ(http_io_threads->getValue(), 1);
  EXPECT_EQ(accept_threads->getValue(), 2);
}