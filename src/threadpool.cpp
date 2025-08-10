#include "threadpool.hpp"

#include <functional>
#include <mutex>
#include <shared_mutex>
#include <thread>

#include "config.hpp"

namespace velox::threadpool
{
  ThreadPool::ThreadPool(const ThreadPoolConfig& config)
      : m_status(Status::RUNNING),
        m_max_task_count(config.max_task_count),
        m_terminating_flag(false),
        m_busy_thread_count(0),
        m_core_thread_count(config.core_thread_count),
        m_max_thread_count(config.max_thread_count),
        m_keep_alive_time(config.keep_alive_time),
        m_monitor_interval(config.monitor_interval)
  {
    if (config.enable_dynamic_scaling)
    {
      m_monitor_thread = std::thread(&ThreadPool::monitorLoop, this);
      VELOX_INFO(
          "ThreadPool dynamic scaling enabled. Monitor active, monitor interval is {}ms",
          std::to_string(config.monitor_interval.count()));
    }
    else
    {
      m_monitor_thread = std::thread{};
      VELOX_INFO("ThreadPool dynamic scaling disabled. Monitor inactive.");
    }

    for (size_t i = 0; i < config.core_thread_count; ++i)
    {
      m_worker_list.emplace_back(this);
    }

    // 向配置模块注册 threadpool, 并添加回调函数
    auto threadpool_config =
        velox::config::Config::getOrCreateConfigVarPtr("threadpool", ThreadPoolConfig(), "threadpool config");

    threadpool_config->addListener(
        [&](const ThreadPoolConfig& old_value, const ThreadPoolConfig& new_value)
        {
          spdlog::info("on_threadpool_conf_changed");

          if (old_value.max_task_count != new_value.max_task_count)
          {
            spdlog::info(
                "ThreadPool max_task_count changed from {} to {}", old_value.max_task_count, new_value.max_task_count);

            m_max_task_count.store(new_value.max_task_count, std::memory_order_release);
          }

          if (old_value.core_thread_count != new_value.core_thread_count)
          {
            spdlog::info(
                "ThreadPool core_thread_count changed from {} to {}",
                old_value.core_thread_count,
                new_value.core_thread_count);

            m_core_thread_count.store(new_value.core_thread_count, std::memory_order_release);
          }

          if (old_value.max_thread_count != new_value.max_thread_count)
          {
            spdlog::info(
                "ThreadPool max_thread_count changed from {} to {}", old_value.max_thread_count, new_value.max_thread_count);

            m_max_thread_count.store(new_value.max_thread_count, std::memory_order_release);
          }

          if (old_value.keep_alive_time != new_value.keep_alive_time)
          {
            spdlog::info(
                "ThreadPool keep_alive_time changed from {}ms to {}ms",
                old_value.keep_alive_time.count(),
                new_value.keep_alive_time.count());

            m_keep_alive_time.store(new_value.keep_alive_time, std::memory_order_release);
          }

          if (old_value.monitor_interval != new_value.monitor_interval)
          {
            spdlog::info(
                "ThreadPool monitor_interval changed from {}ms to {}ms",
                old_value.monitor_interval.count(),
                new_value.monitor_interval.count());

            m_monitor_interval.store(new_value.monitor_interval, std::memory_order_release);
          }
        });
  }

  void ThreadPool::pause()
  {
    std::lock_guard<std::mutex> status_lock(m_status_mutex);

    if (m_status.load(std::memory_order_acquire) == Status::RUNNING)
    {
      m_status.store(Status::PAUSED, std::memory_order_release);
      VELOX_INFO("ThreadPool status: {} -> {}", "RUNNING", "PAUSED");

      // 暂停每一个线程
      std::lock_guard<std::mutex> worker_list_lock(m_worker_list_mutex);
      for (auto& worker : m_worker_list)
      {
        worker.pause();
      }

      // 唤醒全部等待的线程检测自身状态
      m_task_queue_cv.notify_all();
    }
  }

  void ThreadPool::resume()
  {
    std::lock_guard<std::mutex> status_lock(m_status_mutex);
    resumeWithoutStatusLock();
  }

  void ThreadPool::resumeWithoutStatusLock()
  {
    if (m_status.load(std::memory_order_acquire) == Status::PAUSED)
    {
      m_status.store(Status::RUNNING, std::memory_order_release);
      VELOX_INFO("ThreadPool status: {} -> {}", "PAUSED", "RUNNING");

      // 唤醒每一个线程
      std::lock_guard<std::mutex> worker_list_lock(m_worker_list_mutex);
      for (auto& worker : m_worker_list)
      {
        worker.resume();
      }

      // 从 PAUSED 恢复后, 队列中可能有任务, 发送通知信号
      m_task_queue_cv.notify_all();
    }
  }

  void ThreadPool::shutdown()
  {
    // 第一阶段: 设置关闭状态
    {
      std::lock_guard<std::mutex> status_lock(m_status_mutex);

      // 只有当线程池处于 RUNNING 和 PAUSED 态时, 才需要进一步处理
      switch (m_status.load(std::memory_order_acquire))
      {
        case Status::PAUSED: resumeWithoutStatusLock();
        case Status::RUNNING:
        {
          m_status.store(Status::SHUTDOWN, std::memory_order_release);
          VELOX_INFO("ThreadPool status: {} -> {}", "RUNNING", "SHUTDOWN");
          break;
        }
        default: return;  // 其他状态直接返回
      }
    }

    // 第二阶段: 等待任务队列为空
    {
      std::unique_lock<std::mutex> task_queue_lock(m_task_queue_mutex);
      while (!m_task_queue.empty())
      {
        m_task_queue_empty_cv.wait(task_queue_lock);
      }
    }

    // 第三阶段: 进行终止
    m_status.store(Status::TERMINATING, std::memory_order_release);
    m_terminating_flag.store(true, std::memory_order_release);
    m_task_queue_cv.notify_all();  // 唤醒所有等待任务的阻塞线程
    m_monitor_cv.notify_all();     // 唤醒监控线程
    VELOX_INFO("ThreadPool status: {} -> {}", "SHUTDOWN", "TERMINATING");
    VELOX_DEBUG("TEST FLAG 1");

    // 释放全部工作线程
    {
      m_task_queue_cv.notify_all();  // 二次保障, 确保线程均被唤醒
      std::lock_guard<std::mutex> worker_list_lock(m_worker_list_mutex);
      m_worker_list.clear();  // 会阻塞等待全部成员析构完成
    }
    VELOX_DEBUG("TEST FLAG 2");

    // 确保僵尸线程已经释放
    {
      std::lock_guard<std::mutex> zombie_list_lock(m_zombie_workers_mutex);
      m_zombie_workers.clear();  // 对所有被标记删除的线程进行 join
    }
    VELOX_DEBUG("TEST FLAG 3");

    // 等待监控线程结束
    if (m_monitor_thread.joinable())
    {
      m_monitor_thread.join();
    }
    VELOX_DEBUG("TEST FLAG 4");

    m_status.store(Status::TERMINATED, std::memory_order_release);
    VELOX_INFO("ThreadPool status: {} -> {}", "TERMINATING", "TERMINATED");
  }

  void ThreadPool::increaseThreadCount(const std::size_t count)
  {
    std::lock_guard<std::mutex> status_lock(m_status_mutex);
    increaseThreadCountWithoutStatusLock(count);
  }

  void ThreadPool::increaseThreadCountWithoutStatusLock(const std::size_t count)
  {
    const Status current_status = m_status.load(std::memory_order_acquire);

    if (current_status != Status::RUNNING && current_status != Status::PAUSED)
    {
      VELOX_ERROR("Adding threads when thread pool is in an illegal state");
      throw std::logic_error("increaseThreadCount() is only allowed in RUNNING or PAUSED state.");
    }

    std::lock_guard<std::mutex> worker_list_lock(m_worker_list_mutex);
    for (std::size_t i = 0; i < count; ++i)
    {
      m_worker_list.emplace_back(this);
    }
    VELOX_INFO("ThreadPool has increased by {} threads", count);
  }

  void ThreadPool::decreaseThreadCount(const std::size_t count)
  {
    std::lock_guard<std::mutex> status_lock(m_status_mutex);
    decreaseThreadCountWithoutStatusLock(count);
  }

  void ThreadPool::decreaseThreadCountWithoutStatusLock(const std::size_t count)
  {
    const Status current_status = m_status.load(std::memory_order_acquire);

    if (current_status != Status::RUNNING && current_status != Status::PAUSED)
    {
      VELOX_ERROR("Reduce threads when thread pool is in an illegal state");
      throw std::logic_error("drcreaseThreadCount() is only allowed in RUNNING or PAUSED state.");
    }

    // 进行删除操作
    std::lock_guard<std::mutex> worker_list_lock(m_worker_list_mutex);
    std::lock_guard<std::mutex> zombie_list_lock(m_zombie_workers_mutex);

    const std::size_t decrease_count = std::min(count, m_worker_list.size());

    if (decrease_count != 0)
    {
      auto it = m_worker_list.end();
      for (std::size_t i = 0; i < decrease_count; ++i)
      {
        --it;
        it->terminate();
      }

      // 将要删除的线程移动到僵尸列表
      m_zombie_workers.splice(m_zombie_workers.end(), m_worker_list, it, m_worker_list.end());

      m_task_queue_cv.notify_all();  // 唤醒等待任务的线程, 让其检查自身状态
    }

    VELOX_INFO("ThreadPool has decreased by {} threads", decrease_count);
  }

  void ThreadPool::setMaxTaskCount(std::size_t count) { m_max_task_count.store(count, std::memory_order_release); }

  std::size_t ThreadPool::getThreadCount()
  {
    std::lock_guard<std::mutex> worker_list_lock(m_worker_list_mutex);
    return m_worker_list.size();
  }

  std::string ThreadPool::getStatus()
  {
    std::lock_guard<std::mutex> status_lock(m_status_mutex);

    switch (m_status.load(std::memory_order_acquire))
    {
      case Status::RUNNING: return "RUNNING";
      case Status::PAUSED: return "PAUSED";
      case Status::SHUTDOWN: return "SHUTDOWN";
      case Status::TERMINATING: return "TERMINATING";
      case Status::TERMINATED: return "TERMINATED";
      default: return "UNKNOW";
    }
  }

  ThreadPoolConfig ThreadPool::getThreadPoolConfig()
  {
    return {
      m_max_task_count.load(std::memory_order_acquire),   m_core_thread_count.load(std::memory_order_acquire),
      m_max_thread_count.load(std::memory_order_acquire), m_keep_alive_time.load(std::memory_order_acquire),
      m_monitor_interval.load(std::memory_order_acquire),
    };
  }

  bool ThreadPool::isTaskQueueFull()
  {
    const std::size_t max_count = m_max_task_count.load(std::memory_order_acquire);
    if (max_count == 0)
    {
      return false;  // 0 表示不限任务数
    }

    std::lock_guard<std::mutex> lock(m_task_queue_mutex);
    return m_task_queue.size() >= max_count;
  }

  void ThreadPool::monitorLoop()
  {
    while (true)
    {
      // 判断是否终止
      if (m_terminating_flag.load(std::memory_order_acquire))
      {
        break;
      }

      // 进入等待
      {
        std::unique_lock<std::mutex> monitor_lock(m_monitor_mutex);

        // 等待被唤醒
        bool should_terminate = m_monitor_cv.wait_for(
            monitor_lock,
            m_monitor_interval.load(std::memory_order_acquire),
            [this]() { return this->m_terminating_flag.load(std::memory_order_acquire); });

        // 判断是否退出
        if (should_terminate)
        {
          break;
        }
      }

      // 调整工作线程数
      VELOX_INFO("MonitorThread start adjusting the number of worker threads");
      adjustThreadCount();

      // TODO: 可加入定期清理僵尸线程逻辑, 避免 m_zombie_workers 过大
    }

    VELOX_INFO("MonitorThread terminated");
  }

  void ThreadPool::adjustThreadCount()
  {
    std::lock_guard<std::mutex> status_lock(m_status_mutex);  // 状态锁, 避免调整时状态发生变化
    const Status current_status = m_status.load(std::memory_order_acquire);
    // 判断线程池是否处于合法状态
    if (current_status != Status::RUNNING && current_status != Status::PAUSED)
    {
      return;
    }

    // 获取相关数据
    const std::size_t current_threads = getThreadCount();
    const std::size_t busy_threads = m_busy_thread_count.load(std::memory_order_acquire);
    std::size_t queue_size = 0;
    {
      std::lock_guard<std::mutex> task_queue_lock(m_task_queue_mutex);
      queue_size = m_task_queue.size();
    }

    // 扩容策略: 线程池处于运行态, 当前工作线程数量不足以处理当前任务, 存在大量任务积压, 则增加新线程
    // 为了避免线程数量剧烈波动, 一次只增加一个线程
    if (current_status == Status::RUNNING && busy_threads == current_threads && queue_size > 0 &&
        current_threads < m_max_thread_count.load(std::memory_order_acquire))
    {
      increaseThreadCountWithoutStatusLock(1);
      return;
    }

    // 缩容策略: 当线程池中存在超过核心线程数的空闲线程, 且这些空闲线程的空闲时间超过了规定时间
    auto core_thread_count = m_core_thread_count.load(std::memory_order_acquire);
    if (current_threads > core_thread_count && busy_threads < current_threads)
    {
      auto now_time = std::chrono::steady_clock::now();
      std::size_t timeout_thread_count = 0;
      std::size_t non_core_threads_count = current_threads - core_thread_count;

      // 空闲线程检测
      {
        std::lock_guard<std::mutex> worker_list_lock(m_worker_list_mutex);
        auto it = m_worker_list.rbegin();  // 从后往前遍历
        for (std::size_t i = 0; i < non_core_threads_count; ++i, ++it)
        {
          auto last_active_time = it->getLastActiveTime();
          auto idle_duration = std::chrono::duration_cast<std::chrono::milliseconds>(now_time - last_active_time);
          auto keep_alive_time = m_keep_alive_time.load(std::memory_order_acquire);

          if (idle_duration >= keep_alive_time)
          {
            ++timeout_thread_count;
          }
        }
      }

      if (timeout_thread_count > 0)
      {
        decreaseThreadCountWithoutStatusLock(timeout_thread_count);
      }
    }
  }

  // =================================================================================
  // 工作线程实现 (WorkerThread)
  // =================================================================================
  ThreadPool::WorkerThread::WorkerThread(ThreadPool* pool_ptr)
      : m_pool_ptr(pool_ptr),
        m_status(Status::RUNNING),
        m_pause_sem(false),
        m_thread(&ThreadPool::WorkerThread::run, this),
        m_last_active_time(std::chrono::steady_clock::now() - pool_ptr->m_keep_alive_time.load(std::memory_order_acquire))
  {
    VELOX_INFO("Work thread initialization completed");
  }

  ThreadPool::WorkerThread::~WorkerThread()
  {
    if (m_thread.joinable())
    {
      m_thread.join();
    }
  }

  void ThreadPool::WorkerThread::run()
  {
    while (true)
    {
      // --- 阶段1: 检查自身状态 ---
      {
        // 状态互斥量上锁
        std::unique_lock status_lock(m_status_mutex);

        // 处理终止
        if (m_status.load(std::memory_order_acquire) == Status::TERMINATING)
        {
          m_status.store(Status::TERMINATED, std::memory_order_release);
          break;
        }

        // 处理暂停
        if (m_status.load(std::memory_order_acquire) == Status::PAUSED)
        {
          status_lock.unlock();   // 手动释放状态锁
          m_pause_sem.acquire();  // 等待信号量
          continue;
        }
      }

      // --- 阶段2: 处于运行态, 获取任务 ---
      std::function<void()> task;
      {
        // 任务队列上锁
        std::unique_lock queue_lock(m_pool_ptr->m_task_queue_mutex);

        // 判断是否 wait
        while (!isWake())
        {
          m_pool_ptr->m_task_queue_cv.wait(queue_lock);
        }

        /*--------------- 判断被唤醒的原因 ---------------*/
        // 1. 线程处于非运行态
        {
          std::shared_lock status_lock(m_status_mutex);
          if (m_status.load(std::memory_order_acquire) != Status::RUNNING)
          {
            continue;
          }
        }

        // 2. 线程池要停止了, 且队列已空, 则直接准备退出
        if (m_pool_ptr->m_terminating_flag.load(std::memory_order_acquire) && m_pool_ptr->m_task_queue.empty())
        {
          std::unique_lock status_lock(m_status_mutex);
          m_status.store(Status::TERMINATING, std::memory_order_release);
          continue;
        }

        // 3. 线程处于运行态, 对于 终止 + 非空 和 非终止 + 非空 的情况我们都需要取任务执行
        task = std::move(m_pool_ptr->m_task_queue.front());
        m_pool_ptr->m_task_queue.pop();

        if (m_pool_ptr->m_task_queue.empty())
        {
          m_pool_ptr->m_task_queue_empty_cv.notify_all();
        }
      }

      // --- 阶段3: 执行任务(无锁状态) ---
      m_pool_ptr->m_busy_thread_count.fetch_add(1, std::memory_order_release);

      try
      {
        if (task)
        {
          task();  // 执行任务
        }
      }
      catch (const std::exception& e)
      {
        VELOX_ERROR("Task execution failed: {}", e.what());
      }

      m_pool_ptr->m_busy_thread_count.fetch_sub(1, std::memory_order_release);
      m_last_active_time.store(std::chrono::steady_clock::now(), std::memory_order_release);
    }

    VELOX_INFO("WorkerThread terminated");
  }

  void ThreadPool::WorkerThread::terminate()
  {
    std::unique_lock<std::shared_mutex> status_lock(m_status_mutex);
    const Status current_status = m_status.load(std::memory_order_acquire);

    if (current_status == Status::RUNNING || current_status == Status::PAUSED)
    {
      m_status.store(Status::TERMINATING, std::memory_order_release);

      if (current_status == Status::PAUSED)
      {
        m_pause_sem.release();  // 对处于暂停态的线程进行唤醒
      }
    }
  }

  void ThreadPool::WorkerThread::pause()
  {
    std::unique_lock<std::shared_mutex> status_lock(m_status_mutex);

    if (m_status.load(std::memory_order_acquire) == Status::RUNNING)
    {
      m_status.store(Status::PAUSED, std::memory_order_release);
    }
  }

  void ThreadPool::WorkerThread::resume()
  {
    std::unique_lock<std::shared_mutex> status_lock(m_status_mutex);

    if (m_status.load(std::memory_order_acquire) == Status::PAUSED)
    {
      m_status.store(Status::RUNNING, std::memory_order_release);
      m_pause_sem.release();  // 唤醒线程
    }
  }

  std::chrono::steady_clock::time_point ThreadPool::WorkerThread::getLastActiveTime() const
  {
    return m_last_active_time.load(std::memory_order_acquire);
  }

  bool ThreadPool::WorkerThread::isWake()
  {
    // 状态互斥量上读锁, 任务队列锁现在是持有的
    std::shared_lock<std::shared_mutex> status_lock(m_status_mutex);

    // 这里必须使用 m_terminating_flag 不能使用 ThreadPool::getStatus
    // 因为加锁顺序不一致, 会造成死锁
    return (
        m_status.load(std::memory_order_acquire) != Status::RUNNING || !m_pool_ptr->m_task_queue.empty() ||
        m_pool_ptr->m_terminating_flag.load(std::memory_order_acquire));
  }
}  // namespace velox::threadpool

// 实现 ThreadPoolConfig 的偏特化
namespace velox::config
{
  // string -> ThreadPoolConfig
  template<>
  class TypeConverter<std::string, velox::threadpool::ThreadPoolConfig>
  {
   public:
    velox::threadpool::ThreadPoolConfig operator()(const std::string& v)
    {
      YAML::Node node = YAML::Load(v);
      velox::threadpool::ThreadPoolConfig threadpool_config;

      // 该key有默认值, 所以未定义是允许的
      if (node["max_task_count"].IsDefined())
      {
        threadpool_config.max_task_count = node["max_task_count"].as<std::size_t>();
      }

      if (node["core_thread_count"].IsDefined())
      {
        threadpool_config.core_thread_count = node["core_thread_count"].as<std::size_t>();
      }

      if (node["max_thread_count"].IsDefined())
      {
        threadpool_config.max_thread_count = node["max_thread_count"].as<std::size_t>();
      }

      if (node["keep_alive_time"].IsDefined())
      {
        threadpool_config.keep_alive_time = std::chrono::milliseconds(node["keep_alive_time"].as<std::size_t>());
      }

      if (node["monitor_interval"].IsDefined())
      {
        threadpool_config.monitor_interval = std::chrono::milliseconds(node["monitor_interval"].as<std::size_t>());
      }

      return threadpool_config;
    }
  };

  // ThreadPoolConfig -> string
  template<>
  class TypeConverter<velox::threadpool::ThreadPoolConfig, std::string>
  {
   public:
    std::string operator()(const velox::threadpool::ThreadPoolConfig& tpc)
    {
      YAML::Node node;
      node["max_task_count"] = tpc.max_task_count;
      node["core_thread_count"] = tpc.core_thread_count;
      node["max_thread_count"] = tpc.max_thread_count;
      node["keep_alive_time"] = static_cast<std::size_t>(tpc.keep_alive_time.count());
      node["monitor_interval"] = static_cast<std::size_t>(tpc.monitor_interval.count());

      std::stringstream ss;
      ss << node;
      return ss.str();
    }
  };
}  // namespace velox::config