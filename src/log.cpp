#include "log.hpp"

#include <algorithm>
#include <cctype>
#include <iostream>
#include <set>
#include <unordered_map>
#include <vector>

#include "config.hpp"
#include "util.hpp"

namespace velox::log
{
  // 日志输出器类型(强类型枚举)
  enum class AppenderType
  {
    File = 1,
    Stdout,
  };

  // 日志输出器参数
  struct LogAppenderDefine
  {
    AppenderType type = AppenderType::File;
    std::string level;
    std::string formatter;
    std::string file;

    bool operator==(const LogAppenderDefine& oth) const
    {
      return type == oth.type && level == oth.level && formatter == oth.formatter && file == oth.file;
    }
  };

  // 日志器参数
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

    bool operator<(const LogDefine& oth) const { return name < oth.name; }
  };

  // 注册日志器配置参数
  velox::config::ConfigVar<std::set<LogDefine>>::ptr getLogDefinesConfigVar()
  {
    static auto var = velox::config::Config::getOrCreateConfigVarPtr("logs", std::set<LogDefine>(), "log config");
    return var;
  }

  std::filesystem::path getLogPath(const std::string& file)
  {
    const auto& root_path = velox::util::getProjectRootPath();
    std::filesystem::path log_path;

    if (file == "default")
    {
      log_path = root_path / "logs" / "default.log";
    }
    else
    {
      log_path = root_path / file;
    }

    // 确保日志目录存在, 不存在的目录会自动构造
    std::filesystem::create_directories(log_path.parent_path());

    return log_path;
  }

  /**
   * @brief 将 string 转换成对应的 spdlog 日志等级
   */
  spdlog::level::level_enum toSpdlogLevel(const std::string& level_str)
  {
    static const std::unordered_map<std::string, spdlog::level::level_enum> level_map = {
      { "TRACE", spdlog::level::trace }, { "DEBUG", spdlog::level::debug }, { "INFO", spdlog::level::info },
      { "WARN", spdlog::level::warn },   { "ERROR", spdlog::level::err },   { "CRITICAL", spdlog::level::critical },
      { "OFF", spdlog::level::off }
    };

    // 转换为大写(容错处理)
    std::string upper_level = level_str;
    std::transform(
        upper_level.begin(), upper_level.end(), upper_level.begin(), [](unsigned char c) { return std::toupper(c); });

    auto it = level_map.find(upper_level);
    if (it == level_map.end())
    {
      spdlog::info("Invalid log level: {}", level_str);
      throw std::invalid_argument("Invalid log level: " + level_str);
    }

    return it->second;
  }

  /**
   * @brief 根据 LogDefine 创建一个异步 logger
   */
  std::shared_ptr<spdlog::logger> createLoggerFromDefine(const LogDefine& log_def)
  {
    // 处理 appenders
    std::vector<spdlog::sink_ptr> sinks;

    for (const auto& appender : log_def.appenders)
    {
      // appender 只有 File 和 Stdout 两种类型
      if (appender.type == AppenderType::File)
      {
        // File
        auto log_path = getLogPath(appender.file);
        auto file_sink = std::make_shared<spdlog::sinks::daily_file_sink_mt>(log_path.string(), 0, 0);

        if (!appender.level.empty())
        {
          file_sink->set_level(toSpdlogLevel(appender.level));
        }

        if (!appender.formatter.empty())
        {
          file_sink->set_pattern(appender.formatter);
        }

        sinks.push_back(file_sink);
      }
      else
      {
        // Stdcout
        auto stdout_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();

        if (!appender.level.empty())
        {
          stdout_sink->set_level(toSpdlogLevel(appender.level));
        }

        if (!appender.formatter.empty())
        {
          stdout_sink->set_pattern(appender.formatter);
        }

        sinks.push_back(stdout_sink);
      }
    }

    // 创建一个异步日志器, 使用全局线程池
    auto logger = std::make_shared<spdlog::async_logger>(
        log_def.name, sinks.begin(), sinks.end(), spdlog::thread_pool(), spdlog::async_overflow_policy::block);

    // 设置日志器输出级别
    logger->set_level(toSpdlogLevel(log_def.level));

    // 设置日志器输出格式
    logger->set_pattern(log_def.formatter);

    // 设置刷新级别
    logger->flush_on(spdlog::level::warn);

    // 进行注册, 以便通过 spdlog::get() 来获取
    spdlog::register_logger(logger);

    return logger;
  }

  bool initSpdlog(std::size_t queue_size, std::size_t n_threads)
  {
    try
    {
      /*------------- 配置默认的日志器 -------------*/
      spdlog::init_thread_pool(queue_size, n_threads);

      // 标准控制台输出
      auto stdout_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
      stdout_sink->set_level(spdlog::level::debug);

      // 日志文件输出, 0点0分创建新日志
      auto log_path = getLogPath("default");
      auto file_sink = std::make_shared<spdlog::sinks::daily_file_sink_mt>(log_path.string(), 0, 0);
      file_sink->set_level(spdlog::level::info);

      std::vector<spdlog::sink_ptr> sinks{ stdout_sink, file_sink };

      // 创建一个异步日志器
      auto logger = std::make_shared<spdlog::async_logger>(
          "default", sinks.begin(), sinks.end(), spdlog::thread_pool(), spdlog::async_overflow_policy::block);

      // 设置日志器输出阈值
      logger->set_level(spdlog::level::trace);

      // TODO: 添加自定义格式标识符, 用于打印协程ID fiberId(先实现协程库), 参考下面链接实现
      // https://github.com/gabime/spdlog/wiki/Custom-formatting#extending-spdlog-with-your-own-flags
      logger->set_pattern("%^[%Y-%m-%d %T.%e][thread %t][%l][%n][%s:%#]: %v%$");

      // 设置刷新阈值, 当出发 warn 或更严重的错误时立刻刷新日志到  disk
      logger->flush_on(spdlog::level::warn);

      // 每3秒自动刷新依次缓冲区
      spdlog::flush_every(std::chrono::seconds(3));

      // 将其注册为默认 logger
      spdlog::set_default_logger(logger);

      // 设置 spdlog 内部错误处理回调
      spdlog::set_error_handler([](const std::string& msg)
                                { spdlog::log(spdlog::level::critical, "=== SPDLOG LOGGER ERROR ===: {}", msg); });

      /*------------- 设定日志的配置参数 -------------*/
      auto log_defines = getLogDefinesConfigVar();

      // NOTE: 新旧值的交换 setValue() 函数已经完成了
      log_defines->addListener(
          [](const std::set<LogDefine>& old_value, const std::set<LogDefine>& new_value)
          {
            spdlog::info("on_logger_conf_changed");

            // 将旧值和新值转为 map, 方便 diff
            std::unordered_map<std::string, LogDefine> old_map;
            std::unordered_map<std::string, LogDefine> new_map;

            for (const auto& item : old_value)
            {
              old_map[item.name] = item;
            }

            for (const auto& item : new_value)
            {
              new_map[item.name] = item;
            }

            // 处理被删除的日志器
            for (const auto& [name, old_def] : old_map)
            {
              if (new_map.count(name) == 0)
              {
                spdlog::drop(name);
                spdlog::info("Logger [{}] dropped due to config removal", name);
              }
            }

            // 处理新增或更新的日志器
            for (const auto& [name, new_def] : new_map)
            {
              auto old_it = old_map.find(name);

              if (old_it == old_map.end())
              {
                // 新增
                createLoggerFromDefine(new_def);
                spdlog::info("Logger [{}] created from new config", name);
              }
              else
              {
                // 更新
                spdlog::drop(name);  // 卸载旧 logger
                createLoggerFromDefine(new_def);
                spdlog::info("Logger [{}] reloaded due to config change", name);
              }
            }
          });
    }
    catch (const spdlog::spdlog_ex& ex)
    {
      std::cerr << "spdlog initialization failed: " << ex.what() << '\n';
      return false;
    }
    catch (...)
    {
      std::cerr << "spdlog initialization failed: unknown error" << '\n';
      return false;
    }

    return true;
  }

  std::shared_ptr<spdlog::logger> getAsyncLogger(const std::string& name) { return spdlog::get(name); }
}  // namespace velox::log

// 实现 LogDefine 的偏特化
namespace velox::config
{
  // string -> LogDefine
  template<>
  class TypeConverter<std::string, velox::log::LogDefine>
  {
   public:
    velox::log::LogDefine operator()(const std::string& v)
    {
      YAML::Node node = YAML::Load(v);
      velox::log::LogDefine log_def;

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
          velox::log::LogAppenderDefine log_app_def;
          if (type == "FileLogAppender")
          {
            log_app_def.type = velox::log::AppenderType::File;

            if (!appender["file"].IsDefined())
            {
              std::cout << "log config error: fileappender file is null, " << YAML::Dump(appender) << std::endl;
              continue;
            }

            log_app_def.file = appender["file"].as<std::string>();
          }
          else if (type == "StdoutLogAppender")
          {
            log_app_def.type = velox::log::AppenderType::Stdout;
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
  class TypeConverter<velox::log::LogDefine, std::string>
  {
   public:
    std::string operator()(const velox::log::LogDefine& ld)
    {
      YAML::Node node;
      node["name"] = ld.name;
      node["level"] = ld.level;
      node["formatter"] = ld.formatter;

      for (const auto& app : ld.appenders)
      {
        YAML::Node app_node;

        if (app.type == velox::log::AppenderType::File)
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