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
  // 测试复杂一点的情况
  auto hosts_var = Config::getOrCreateConfigVarPtr<std::vector<std::string>>("server.hosts", { "localhost" });
  ASSERT_NE(hosts_var, nullptr);  // 确保复杂类型创建成功
  auto hosts_var_int_vec = Config::getOrCreateConfigVarPtr<std::vector<int>>("server.hosts", { 127 });
  EXPECT_EQ(hosts_var_int_vec, nullptr);

  // 4. 非法名称
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

TEST_F(ConfigTest, loadFromYaml)
{
  // 注册配置变量
  auto servers0_address0 = Config::getOrCreateConfigVarPtr<std::string>("servers.0.address.0", "");
  auto servers0_address1 = Config::getOrCreateConfigVarPtr<std::string>("servers.0.address.1", "");
  auto servers0_address2 = Config::getOrCreateConfigVarPtr<std::string>("servers.0.address.2", "");
  auto servers0_keepalive = Config::getOrCreateConfigVarPtr<int>("servers.0.keepalive", 0);
  auto servers0_timeout = Config::getOrCreateConfigVarPtr<int>("servers.0.timeout", 0);
  auto servers0_name = Config::getOrCreateConfigVarPtr<std::string>("servers.0.name", "");
  auto servers0_accept_worker = Config::getOrCreateConfigVarPtr<std::string>("servers.0.accept_worker", "");
  auto servers0_io_worker = Config::getOrCreateConfigVarPtr<std::string>("servers.0.io_worker", "");
  auto servers0_process_worker = Config::getOrCreateConfigVarPtr<std::string>("servers.0.process_worker", "");
  auto servers0_type = Config::getOrCreateConfigVarPtr<std::string>("servers.0.type", "");

  auto servers1_address0 = Config::getOrCreateConfigVarPtr<std::string>("servers.1.address.0", "");
  auto servers1_address1 = Config::getOrCreateConfigVarPtr<std::string>("servers.1.address.1", "");
  auto servers1_timeout = Config::getOrCreateConfigVarPtr<int>("servers.1.timeout", 0);
  auto servers1_name = Config::getOrCreateConfigVarPtr<std::string>("servers.1.name", "");
  auto servers1_accept_worker = Config::getOrCreateConfigVarPtr<std::string>("servers.1.accept_worker", "");
  auto servers1_io_worker = Config::getOrCreateConfigVarPtr<std::string>("servers.1.io_worker", "");
  auto servers1_process_worker = Config::getOrCreateConfigVarPtr<std::string>("servers.1.process_worker", "");
  auto servers1_type = Config::getOrCreateConfigVarPtr<std::string>("servers.1.type", "");

  // 构造 YAML 内容
  std::string yaml_content = R"(servers:
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

  // 检查第一个 server
  {
    EXPECT_EQ(servers0_address0->getValue(), "0.0.0.0:8090");
    EXPECT_EQ(servers0_address1->getValue(), "127.0.0.1:8091");
    EXPECT_EQ(servers0_address2->getValue(), "/tmp/test.sock");
    EXPECT_EQ(servers0_keepalive->getValue(), 1);
    EXPECT_EQ(servers0_timeout->getValue(), 1000);
    EXPECT_EQ(servers0_name->getValue(), "sylar/1.1");
    EXPECT_EQ(servers0_accept_worker->getValue(), "accept");
    EXPECT_EQ(servers0_io_worker->getValue(), "http_io");
    EXPECT_EQ(servers0_process_worker->getValue(), "http_io");
    EXPECT_EQ(servers0_type->getValue(), "http");
  }

  // 检查第二个 server
  {
    EXPECT_EQ(servers1_address0->getValue(), "0.0.0.0:8062");
    EXPECT_EQ(servers1_address1->getValue(), "0.0.0.0:8061");
    EXPECT_EQ(servers1_timeout->getValue(), 1000);
    EXPECT_EQ(servers1_name->getValue(), "sylar-rock/1.0");
    EXPECT_EQ(servers1_accept_worker->getValue(), "accept");
    EXPECT_EQ(servers1_io_worker->getValue(), "io");
    EXPECT_EQ(servers1_process_worker->getValue(), "io");
    EXPECT_EQ(servers1_type->getValue(), "rock");
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
  auto s0_address_0 = Config::getOrCreateConfigVarPtr<std::string>("servers.0.address.0", {});
  auto s0_address_1 = Config::getOrCreateConfigVarPtr<std::string>("servers.0.address.1", {});
  auto s0_address_2 = Config::getOrCreateConfigVarPtr<std::string>("servers.0.address.2", {});
  auto s0_keepalive = Config::getOrCreateConfigVarPtr<int>("servers.0.keepalive", 0);
  auto s0_timeout = Config::getOrCreateConfigVarPtr<int>("servers.0.timeout", 0);
  auto s0_name = Config::getOrCreateConfigVarPtr<std::string>("servers.0.name", "");
  auto s0_accept_worker = Config::getOrCreateConfigVarPtr<std::string>("servers.0.accept_worker", "");
  auto s0_io_worker = Config::getOrCreateConfigVarPtr<std::string>("servers.0.io_worker", "");
  auto s0_process_worker = Config::getOrCreateConfigVarPtr<std::string>("servers.0.process_worker", "");
  auto s0_type = Config::getOrCreateConfigVarPtr<std::string>("servers.0.type", "");

  auto s1_address_0 = Config::getOrCreateConfigVarPtr<std::string>("servers.1.address.0", {});
  auto s1_address_1 = Config::getOrCreateConfigVarPtr<std::string>("servers.1.address.1", {});
  auto s1_timeout = Config::getOrCreateConfigVarPtr<int>("servers.1.timeout", 0);
  auto s1_name = Config::getOrCreateConfigVarPtr<std::string>("servers.1.name", "");
  auto s1_accept_worker = Config::getOrCreateConfigVarPtr<std::string>("servers.1.accept_worker", "");
  auto s1_io_worker = Config::getOrCreateConfigVarPtr<std::string>("servers.1.io_worker", "");
  auto s1_process_worker = Config::getOrCreateConfigVarPtr<std::string>("servers.1.process_worker", "");
  auto s1_type = Config::getOrCreateConfigVarPtr<std::string>("servers.1.type", "");

  /* ----------------------- 注册 log.yml 配置  -----------------------*/
  auto log0_name = Config::getOrCreateConfigVarPtr<std::string>("logs.0.name", "");
  auto log0_level = Config::getOrCreateConfigVarPtr<std::string>("logs.0.level", "");
  auto log0_app0_type = Config::getOrCreateConfigVarPtr<std::string>("logs.0.appenders.0.type", "");
  auto log0_app0_file = Config::getOrCreateConfigVarPtr<std::string>("logs.0.appenders.0.file", "");
  auto log0_app1_type = Config::getOrCreateConfigVarPtr<std::string>("logs.0.appenders.1.type", "");

  auto log1_name = Config::getOrCreateConfigVarPtr<std::string>("logs.1.name", "");
  auto log1_level = Config::getOrCreateConfigVarPtr<std::string>("logs.1.level", "");
  auto log1_app0_type = Config::getOrCreateConfigVarPtr<std::string>("logs.1.appenders.0.type", "");
  auto log1_app0_file = Config::getOrCreateConfigVarPtr<std::string>("logs.1.appenders.0.file", "");
  auto log1_app1_type = Config::getOrCreateConfigVarPtr<std::string>("logs.1.appenders.1.type", "");

  // 加载指定目录下的配置文件
  Config::loadFromConfDir("test/config");

  // 验证 log.yml是否正确加载
  {
    EXPECT_EQ(log0_name->getValue(), "root");
    EXPECT_EQ(log0_level->getValue(), "info");
    EXPECT_EQ(log0_app0_type->getValue(), "FileLogAppender");
    EXPECT_EQ(log0_app0_file->getValue(), "/apps/logs/sylar/root.txt");
    EXPECT_EQ(log0_app1_type->getValue(), "StdoutLogAppender");

    EXPECT_EQ(log1_name->getValue(), "system");
    EXPECT_EQ(log1_level->getValue(), "info");
    EXPECT_EQ(log1_app0_type->getValue(), "FileLogAppender");
    EXPECT_EQ(log1_app0_file->getValue(), "/apps/logs/sylar/system.txt");
    EXPECT_EQ(log1_app1_type->getValue(), "StdoutLogAppender");
  }

  // 验证 server.yml是否正确加载
  {
    EXPECT_EQ(s0_address_0->getValue(), "0.0.0.0:8090");
    EXPECT_EQ(s0_address_1->getValue(), "127.0.0.1:8091");
    EXPECT_EQ(s0_address_2->getValue(), "/tmp/test.sock");
    EXPECT_EQ(s0_keepalive->getValue(), 1);
    EXPECT_EQ(s0_timeout->getValue(), 1000);
    EXPECT_EQ(s0_name->getValue(), "sylar/1.1");
    EXPECT_EQ(s0_accept_worker->getValue(), "accept");
    EXPECT_EQ(s0_io_worker->getValue(), "http_io");
    EXPECT_EQ(s0_process_worker->getValue(), "http_io");
    EXPECT_EQ(s0_type->getValue(), "http");

    EXPECT_EQ(s1_address_0->getValue(), "0.0.0.0:8062");
    EXPECT_EQ(s1_address_1->getValue(), "0.0.0.0:8061");
    EXPECT_EQ(s1_timeout->getValue(), 1000);
    EXPECT_EQ(s1_name->getValue(), "sylar-rock/1.0");
    EXPECT_EQ(s1_accept_worker->getValue(), "accept");
    EXPECT_EQ(s1_io_worker->getValue(), "io");
    EXPECT_EQ(s1_process_worker->getValue(), "io");
    EXPECT_EQ(s1_type->getValue(), "rock");
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