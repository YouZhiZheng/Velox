#include "log.hpp"

#include <iostream>

namespace velox::log
{
  std::filesystem::path getLogPath(const std::string& name)
  {
    static const std::filesystem::path root(PROJECT_ROOT_DIR);
    std::filesystem::path log_dir;

    if (name == "default")
    {
      log_dir = root / "logs";
    }
    else
    {
      log_dir = root / "logs" / name;
    }

    std::filesystem::create_directories(log_dir);
    return (log_dir / (name + ".log"));
  }

  bool initSpdlog(std::size_t queue_size, std::size_t n_threads)
  {
    try
    {
      spdlog::init_thread_pool(queue_size, n_threads);

      // 标准控制台输出
      auto stdout_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
      stdout_sink->set_level(spdlog::level::debug);

      // 日志文件输出，0点0分创建新日志
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
      // 设置输出格式, 详细可参考 https://github.com/gabime/spdlog/wiki/Custom-formatting#pattern-flags
      // eg. [2025-07-03 11:17:53.345][thread Id][fiber id][debug][Logger][main.cpp:37]: ......
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

  std::shared_ptr<spdlog::logger> getAsyncFileLogger(const std::string& name)
  {
    // 查询该logger是否存在
    auto log = spdlog::get(name);

    if (!log)
    {
      // 指针为空，则创建该日志记录器
      auto log_path = getLogPath(name);
      log = spdlog::daily_logger_mt<spdlog::async_factory>(name, log_path.string());
      log->set_level(spdlog::level::trace);
      log->flush_on(spdlog::level::err);
      log->set_pattern("[%Y-%m-%d %T.%e][thread %t][%l][%n][%s:%#]: %v");
      // 记录器是自动注册的，不需要手动注册  spdlog::register_logger(name);
    }

    return log;
  }
}  // namespace velox::log