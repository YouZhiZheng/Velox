/**
 * @file log.hpp
 * @brief 日志模块封装
 * @author zhengyouzhi
 * @email youzhizheng9@gmail.com
 * @date 2025-7-1
 * @copyright Copyright (c) 2025 zhengyouzhi
 * @license   GNU General Public License v3.0
 *            See LICENSE file in the project root for full license information.
 */

#pragma once

#include <spdlog/async.h>
#include <spdlog/sinks/daily_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#include <spdlog/stopwatch.h>

#include <filesystem>
#include <string>

// 判断CMake是否生成了 PROJECT_ROOT_DIR 宏定义
#ifndef PROJECT_ROOT_DIR
#define PROJECT_ROOT_DIR ""
#endif

namespace velox::log
{
  constexpr std::size_t DEFAULT_QUEUE_SIZE = 32768;
  constexpr std::size_t DEFAULT_THREAD_NUM = 1;

  /**
   * @brief 对 spdlog 进行初始化, 最终会创建一个默认异步构造器
   * @param[in] queue_size 用于异步 logger 的队列大小
   * @param[in] n_threads 用于异步 logger 的线程数
   * @return 成功返回 true
   */
  bool initSpdlog(std::size_t queue_size = DEFAULT_QUEUE_SIZE, std::size_t n_threads = DEFAULT_THREAD_NUM);

  /**
   * @brief 获取或创建一个异步logger
   * @param[in] name 要创建或获取的 logger 名字
   * @return 返回对应的logger
   */
  std::shared_ptr<spdlog::logger> getAsyncFileLogger(const std::string& name);

  /**
   * @brief 获取指定 logger 的日志文件存储路径
   * @param[in] name logger 名字
   * @return 返回该 logger 的日志文件的绝对存储路径
   */
  std::filesystem::path getLogPath(const std::string& name);

  // /**
  //  * @brief 获取相对于项目根目录的路径
  //  * @param[in] full_path 完整路径
  //  */
  // inline constexpr std::string_view getRelativePath(std::string_view full_path)
  // {
  //   constexpr auto root_len = sizeof(PROJECT_ROOT_DIR) - 1;
  //   if ((full_path.length() > root_len) && (full_path.substr(0, root_len) == PROJECT_ROOT_DIR) && full_path[root_len] ==
  //   '/')
  //   {
  //     return full_path.substr(root_len + 1);
  //   }
  //   return full_path;
  // }
}  // namespace velox::log

/**
 * @brief 使用默认参数初始化
 */
#define VELOX_LOG_INIT() velox::log::initSpdlog()

/**
 * @brief 关闭日志系统
 */
#define VELOX_LOG_SHUTDOWN() spdlog::shutdown();

/**
 * @brief 使用默认日志器进行输出, 输出到控制台和文件
 */
#define VELOX_TRACE(...)    SPDLOG_TRACE(__VA_ARGS__)
#define VELOX_DEBUG(...)    SPDLOG_DEBUG(__VA_ARGS__)
#define VELOX_INFO(...)     SPDLOG_INFO(__VA_ARGS__)
#define VELOX_WARN(...)     SPDLOG_WARN(__VA_ARGS__)
#define VELOX_ERROR(...)    SPDLOG_ERROR(__VA_ARGS__)
#define VELOX_CRITICAL(...) SPDLOG_CRITICAL(__VA_ARGS__)

/**
 * @brief 获取指定 logger
 */
#define VELOX_GETLOG(LOG_NAME) velox::log::getAsyncFileLogger(LOG_NAME)

/**
 * @brief 使用指定日志器进行输出, 输出到文件
 */
#define VELOX_LOGGER_TRACE(logger, ...)    SPDLOG_LOGGER_TRACE(logger, __VA_ARGS__)
#define VELOX_LOGGER_DEBUG(logger, ...)    SPDLOG_LOGGER_DEBUG(logger, __VA_ARGS__)
#define VELOX_LOGGER_INFO(logger, ...)     SPDLOG_LOGGER_INFO(logger, __VA_ARGS__)
#define VELOX_LOGGER_WARN(logger, ...)     SPDLOG_LOGGER_WARN(logger, __VA_ARGS__)
#define VELOX_LOGGER_ERROR(logger, ...)    SPDLOG_LOGGER_ERROR(logger, __VA_ARGS__)
#define VELOX_LOGGER_CRITICAL(logger, ...) SPDLOG_LOGGER_CRITICAL(logger, __VA_ARGS__)

/**
 * @brief 获取一个计时器
 */
#define VELOX_LOGSW() spdlog::stopwatch()
