/**
 * @file log_test.cpp
 * @brief 日志模块测试
 * @author zhengyouzhi
 * @email youzhizheng9@gmail.com
 * @date 2025-7-5
 * @copyright Copyright (c) 2025 zhengyouzhi
 * @license   GNU General Public License v3.0
 *            See LICENSE file in the project root for full license information.
 */

#include "log.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <thread>

/**
 * @brief 清除指定目录下的子目录和文件
 * @param[in] dir 指定目录路径
 */
void clearLogDirectory(const std::filesystem::path& dir)
{
  if (!std::filesystem::exists(dir) || !std::filesystem::is_directory(dir))
  {
    return;
  }

  std::error_code error_cd;
  for (const auto& entry : std::filesystem::directory_iterator(dir, error_cd))
  {
    if (error_cd)
    {
      std::cerr << "Failed to iterate directory: " << dir << ", error: " << error_cd.message() << '\n';
      return;
    }

    std::filesystem::remove_all(entry.path(), error_cd);
    if (error_cd)
    {
      std::cerr << "Failed to remove: " << entry.path() << ", error: " << error_cd.message() << '\n';
    }
  }
}

/**
 * @brief 将指定文件的内容转化成 string 返回
 * @param[in] file_path 文件路径
 */
std::string readFileToString(const std::filesystem::path& file_path)
{
  std::ifstream file(file_path);
  if (!file.is_open())
  {
    return {};
  }

  return { std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>() };
}

/**
 * @brief 获取当前日期, 用于构造日志文件名称
 */
std::string getCurrentDateStr()
{
  auto now = std::time(nullptr);
  std::tm tm_now{};
  localtime_r(&now, &tm_now);
  std::ostringstream oss;
  oss << std::put_time(&tm_now, "%Y-%m-%d");
  return oss.str();
}

class LogTest : public ::testing::Test
{
 protected:
  void SetUp() override
  {
    // 每个测试开始前初始化
    bool init_result = VELOX_LOG_INIT();
    ASSERT_TRUE(init_result);
  }

  void TearDown() override
  {
    // 每个测试结束后关闭日志系统
    VELOX_LOG_SHUTDOWN();

    // 清理 logs 目录
    const std::filesystem::path root(PROJECT_ROOT_DIR);
    std::filesystem::path log_dir = root / "logs";
    clearLogDirectory(log_dir);
  }
};

// 对默认 logger 进行测试
TEST_F(LogTest, DefaultLogger)
{
  // 单线程环境
  {
    VELOX_TRACE("Test trace message from default logger");
    VELOX_DEBUG("Test debug message from default logger");
    VELOX_INFO("Test info message from default logger");
    VELOX_WARN("Test warn message from default logger");
    VELOX_ERROR("Test error message from default logger");
    VELOX_CRITICAL("Test critical message from default logger");

    // 判断日志文件是否存在
    auto default_log_path = velox::log::getLogPath("default");
    default_log_path = default_log_path.parent_path() / ("default_" + getCurrentDateStr() + ".log");
    ASSERT_TRUE(std::filesystem::exists(default_log_path)) << "Log file does not exist: " << default_log_path;

    std::this_thread::sleep_for(std::chrono::seconds(3));  // 等待3秒, 以便全部内容写入完成

    std::string content = readFileToString(default_log_path);
    EXPECT_EQ(content.find("Test trace message"), std::string::npos);  // 默认 logger 的阈值为 info
    EXPECT_EQ(content.find("Test debug message"), std::string::npos);
    EXPECT_NE(content.find("Test info message"), std::string::npos);
    EXPECT_NE(content.find("Test warn message"), std::string::npos);
    EXPECT_NE(content.find("Test error message"), std::string::npos);
    EXPECT_NE(content.find("Test critical message"), std::string::npos);
  }

  // 多线程环境
  {
    constexpr int thread_count = 6;
    constexpr int logs_per_thread = 100;

    std::vector<std::thread> threads;
    threads.reserve(thread_count);
    for (int i = 0; i < thread_count; ++i)
    {
      threads.emplace_back(
          [i]()
          {
            for (int j = 0; j < logs_per_thread; ++j)
            {
              VELOX_INFO("Thread {} writes log {}", i, j);
            }
          });
    }

    // 等待每个线程完成
    for (auto& thread : threads)
    {
      thread.join();
    }

    // 判断日志文件是否存在
    auto default_log_path = velox::log::getLogPath("default");
    default_log_path = default_log_path.parent_path() / ("default_" + getCurrentDateStr() + ".log");
    ASSERT_TRUE(std::filesystem::exists(default_log_path)) << "Log file does not exist: " << default_log_path;

    std::this_thread::sleep_for(std::chrono::seconds(3));  // 等待3秒, 以便全部内容写入完成

    std::string content = readFileToString(default_log_path);
    EXPECT_NE(content.find("Thread 0 writes log 0"), std::string::npos);
    EXPECT_NE(content.find("Thread 0 writes log 99"), std::string::npos);
    EXPECT_NE(content.find("Thread 1 writes log 0"), std::string::npos);
    EXPECT_NE(content.find("Thread 1 writes log 99"), std::string::npos);
    EXPECT_NE(content.find("Thread 2 writes log 0"), std::string::npos);
    EXPECT_NE(content.find("Thread 2 writes log 99"), std::string::npos);
    EXPECT_NE(content.find("Thread 3 writes log 0"), std::string::npos);
    EXPECT_NE(content.find("Thread 3 writes log 99"), std::string::npos);
    EXPECT_NE(content.find("Thread 4 writes log 0"), std::string::npos);
    EXPECT_NE(content.find("Thread 4 writes log 99"), std::string::npos);
    EXPECT_NE(content.find("Thread 5 writes log 0"), std::string::npos);
    EXPECT_NE(content.find("Thread 5 writes log 99"), std::string::npos);
  }
}

// 对异步 logger 进行测试
TEST_F(LogTest, AsyncFileLogger)
{
  // 单线程
  {
    auto logger = VELOX_GETLOG("test1");

    VELOX_LOGGER_TRACE(logger, "Test trace message from default logger");
    VELOX_LOGGER_DEBUG(logger, "Test debug message from default logger");
    VELOX_LOGGER_INFO(logger, "Test info message from default logger");
    VELOX_LOGGER_WARN(logger, "Test warn message from default logger");
    VELOX_LOGGER_ERROR(logger, "Test error message from default logger");
    VELOX_LOGGER_CRITICAL(logger, "Test critical message from default logger");

    // 判断日志文件是否存在
    auto log_path = velox::log::getLogPath("test1");
    log_path = log_path.parent_path() / ("test1_" + getCurrentDateStr() + ".log");
    ASSERT_TRUE(std::filesystem::exists(log_path)) << "Log file does not exist: " << log_path;

    std::this_thread::sleep_for(std::chrono::seconds(3));  // 等待3秒, 以便全部内容写入完成

    // 异步logger的阈值默认为trace
    std::string content = readFileToString(log_path);
    EXPECT_NE(content.find("Test trace message"), std::string::npos);
    EXPECT_NE(content.find("Test debug message"), std::string::npos);
    EXPECT_NE(content.find("Test info message"), std::string::npos);
    EXPECT_NE(content.find("Test warn message"), std::string::npos);
    EXPECT_NE(content.find("Test error message"), std::string::npos);
    EXPECT_NE(content.find("Test critical message"), std::string::npos);
  }

  // 多线程
  {
    auto logger = VELOX_GETLOG("test2");

    constexpr int thread_count = 6;
    constexpr int logs_per_thread = 100;

    std::vector<std::thread> threads;
    threads.reserve(thread_count);
    for (int i = 0; i < thread_count; ++i)
    {
      threads.emplace_back(
          [&logger, i]()
          {
            for (int j = 0; j < logs_per_thread; ++j)
            {
              VELOX_LOGGER_INFO(logger, "Thread {} writes log {}", i, j);
            }
          });
    }

    // 等待每个线程完成
    for (auto& thread : threads)
    {
      thread.join();
    }

    // 判断日志文件是否存在
    auto log_path = velox::log::getLogPath("test2");
    log_path = log_path.parent_path() / ("test2_" + getCurrentDateStr() + ".log");
    ASSERT_TRUE(std::filesystem::exists(log_path)) << "Log file does not exist: " << log_path;

    std::this_thread::sleep_for(std::chrono::seconds(3));  // 等待3秒, 以便全部内容写入完成

    std::string content = readFileToString(log_path);
    EXPECT_NE(content.find("Thread 0 writes log 0"), std::string::npos);
    EXPECT_NE(content.find("Thread 0 writes log 99"), std::string::npos);
    EXPECT_NE(content.find("Thread 1 writes log 0"), std::string::npos);
    EXPECT_NE(content.find("Thread 1 writes log 99"), std::string::npos);
    EXPECT_NE(content.find("Thread 2 writes log 0"), std::string::npos);
    EXPECT_NE(content.find("Thread 2 writes log 99"), std::string::npos);
    EXPECT_NE(content.find("Thread 3 writes log 0"), std::string::npos);
    EXPECT_NE(content.find("Thread 3 writes log 99"), std::string::npos);
    EXPECT_NE(content.find("Thread 4 writes log 0"), std::string::npos);
    EXPECT_NE(content.find("Thread 4 writes log 99"), std::string::npos);
    EXPECT_NE(content.find("Thread 5 writes log 0"), std::string::npos);
    EXPECT_NE(content.find("Thread 5 writes log 99"), std::string::npos);
  }
}