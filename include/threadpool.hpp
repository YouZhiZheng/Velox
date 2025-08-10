/**
 * @file threadpool.hpp
 * @brief 线程池模块
 * @author zhengyouzhi
 * @email youzhizheng9@gmail.com
 * @date 2025-7-22
 * @copyright Copyright (c) 2025 zhengyouzhi
 * @license   GNU General Public License v3.0
 *            See LICENSE file in the project root for full license information.
 */

#pragma once

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <future>
#include <list>
#include <mutex>
#include <queue>
#include <shared_mutex>
#include <string>
#include <thread>

#include "log.hpp"

namespace velox::threadpool
{
  // 手动实现的一个简单 二进制信号量
  class BinarySemaphore
  {
   public:
    explicit BinarySemaphore(bool initially_available = false) : m_flag(initially_available) { }

    void acquire()
    {
      std::unique_lock<std::mutex> lock(m_mutex);
      m_cv.wait(lock, [this] { return m_flag; });
      m_flag = false;  // 消耗信号量
    }

    void release()
    {
      std::unique_lock<std::mutex> lock(m_mutex);
      if (!m_flag)
      {
        m_flag = true;
        m_cv.notify_one();  // 唤醒线程
      }
    }

   private:
    std::mutex m_mutex;
    std::condition_variable m_cv;
    bool m_flag;
  };

  struct ThreadPoolConfig
  {
    std::size_t max_task_count = 0;                     // NOLINT
    std::size_t core_thread_count = 1;                  // NOLINT
    std::size_t max_thread_count = 8;                   // NOLINT
    std::chrono::milliseconds keep_alive_time{ 5000 };  // NOLINT
    std::chrono::milliseconds monitor_interval{ 200 };  // NOLINT
    bool enable_dynamic_scaling = true;

    bool operator==(const ThreadPoolConfig& oth) const
    {
      return (
          max_task_count == oth.max_task_count && core_thread_count == oth.core_thread_count &&
          max_thread_count == oth.max_thread_count && keep_alive_time == oth.keep_alive_time &&
          monitor_interval == oth.monitor_interval);
    }
  };

  class ThreadPool
  {
   private:
    class WorkerThread;  // 工作线程类

    // 线程池运行状态定义
    enum class Status : std::uint8_t
    {
      RUNNING,      // 运行中: 接收并处理任务
      PAUSED,       // 暂停: 接收任务但不处理
      SHUTDOWN,     // 关闭中: 不再接收新任务, 等待剩余任务完成
      TERMINATING,  // 终止中: 释放资源
      TERMINATED    // 终止: 所有工作线程已退出, 资源已释放, 不可再使用
    };

    // 线程池名称
    const std::string m_name;

    // 状态相关变量
    std::atomic<Status> m_status;
    std::mutex m_status_mutex;

    // 任务队列相关变量
    std::atomic<std::size_t> m_max_task_count;
    std::queue<std::function<void()>> m_task_queue;
    std::mutex m_task_queue_mutex;
    std::condition_variable m_task_queue_cv;
    std::condition_variable m_task_queue_empty_cv;

    // 工作线程相关变量
    std::list<WorkerThread> m_worker_list;
    std::mutex m_worker_list_mutex;

    // 僵尸线程相关变量
    std::list<WorkerThread> m_zombie_workers;
    std::mutex m_zombie_workers_mutex;

    // 线程池终止标志
    std::atomic<bool> m_terminating_flag;

    // 监控线程相关变量
    std::thread m_monitor_thread;
    std::mutex m_monitor_mutex;
    std::condition_variable m_monitor_cv;
    std::atomic<std::size_t> m_busy_thread_count;
    std::atomic<std::size_t> m_core_thread_count;               // 默认1
    std::atomic<std::size_t> m_max_thread_count;                // 默认8
    std::atomic<std::chrono::milliseconds> m_keep_alive_time;   // 默认 5000 ms
    std::atomic<std::chrono::milliseconds> m_monitor_interval;  // 默认 200 ms

    bool isTaskQueueFull();
    void resumeWithoutStatusLock();
    void increaseThreadCountWithoutStatusLock(std::size_t count);
    void decreaseThreadCountWithoutStatusLock(std::size_t count);

    /**
     * @brief 监控线程要执行的函数
     * @details 周期性的判断线程池是否需要 增加/删除 工作线程
     */
    void monitorLoop();

    /**
     * @brief 自动调整工作线程数
     */
    void adjustThreadCount();

   public:
    // 禁用拷贝/移动构造函数及赋值运算符
    ThreadPool(const ThreadPool& other) = delete;
    ThreadPool(ThreadPool&& other) noexcept = delete;
    ThreadPool& operator=(const ThreadPool& other) = delete;
    ThreadPool& operator=(ThreadPool&& other) noexcept = delete;

    /**
     * @brief 构造函数
     * @param[in]  initial_thread_count 线程池中的初始线程数
     * @param[in] max_task_count 任务队列能存储的最大任务数, 默认值 0 表示不限制
     */
    explicit ThreadPool(const ThreadPoolConfig& config);

    /**
     * @brief 析构函数
     */
    ~ThreadPool() { shutdown(); };

    /**
     * @brief 使线程池从运行态进入暂停态
     * @details 暂停态的线程池只接收任务, 并不处理任务
     */
    void pause();

    /**
     * @brief 使线程池从暂停态恢复至运行态
     */
    void resume();

    /**
     * @brief 优雅的关闭线程池
     * @details 不再接收新任务, 等待所有已提交任务完成后优雅的关闭线程池.
     *          如果线程池处于 PAUSED 态, 会先恢复至运行态.
     *          该函数会等待全部线程都结束.
     */
    void shutdown();

    /**
     * @brief 动态增加线程池中的工作线程数量
     * @note 只有线程池处于 RUNNING 和 PAUSED 时才可调用
     */
    void increaseThreadCount(std::size_t count);

    /**
     * @brief 动态移除线程池中的部分工作线程
     * @note 只有线程池处于 RUNNING 和 PAUSED 时才可调用, 且只发生结束信号给工作线程,
     *       不等待工作线程真正结束
     */
    void decreaseThreadCount(std::size_t count);

    /**
     * @brief 设置任务队列的最大容量限制(0表示不限制)
     */
    void setMaxTaskCount(std::size_t count);

    /**
     * @brief 获取当前线程池中活跃的工作线程数量
     */
    [[nodiscard]] std::size_t getThreadCount();

    /**
     * @brief 获取线程池状态
     */
    [[nodiscard]] std::string getStatus();

    /**
     * @brief 获取线程池配置参数
     */
    [[nodiscard]] ThreadPoolConfig getThreadPoolConfig();

    /**
     * @brief 提交任务到任务队列, 并返回一个 future 对象以获取执行结果
     * @details 支持任意函数签名及参数组合. 内部将任务封装为无参的 `std::function<void()>`
     *          形式, 统一工作线程的任务调度格式. 任务将在后台线程异步执行.
     *          暂停态也可以提交任务, 但不会执行新任务
     *
     * @note 如果某个参数是引用类型, 则在提交任务时要用 std::ref(data) 的形式来明确告知submit函数传引用
     *
     * @tparam F 可调用对象的类型(函数指针, lambda, std::function等)
     * @tparam Args 可调用对象 f 所需的参数类型
     *
     * @param[in] f 要执行的函数对象
     * @param[in] args 要传递给函数 f 的参数(可变参数包)
     * @return 返回一个 future 对象
     */
    template<typename F, typename... Args>
    std::future<std::invoke_result_t<F, Args...>> submit(F&& f, Args&&... args)
    {
      // 加锁, 防止在提交任务过程中线程池状态发生变化
      std::lock_guard<std::mutex> status_lock(m_status_mutex);
      const Status current_status = m_status.load(std::memory_order_acquire);

      // 检查当前线程池状态
      if (current_status != Status::RUNNING && current_status != Status::PAUSED)
      {
        VELOX_ERROR("[submit error]: ThreadPool is in a state where it cannot submit tasks");
        throw std::runtime_error("[submit error]: ThreadPool is in a state where it cannot submit tasks");
      }

      // 判断任务队列是否已满
      if (isTaskQueueFull())
      {
        VELOX_ERROR("[submit error]: task queue is full");
        throw std::runtime_error("[submit error]: task queue is full");
      }

      using return_type = std::invoke_result_t<F, Args...>;
      auto args_tuple = std::make_tuple(std::forward<Args>(args)...);
      // 将提交的任务进行双重包装:
      // 1. 内层 lambda 将 f 和其参数封装成一个无参可调用的对象. 统一了工作线程统执行任务的方式(通过()来执行)
      // 2. 外层 std::packaged_task 封装该 lambda. 这样做能够获取 future, 以便任务提交者能够获取执行结果
      auto task_ptr = std::make_shared<std::packaged_task<return_type()>>(
          [f = std::forward<F>(f), tuple = std::move(args_tuple)]() mutable -> return_type { return std::apply(f, tuple); });

      // 获取该任务的 future, 通过该变量获取任务执行结果
      std::future<return_type> future = task_ptr->get_future();

      // 将任务提交到任务队列中
      {
        std::lock_guard<std::mutex> lock(m_task_queue_mutex);
        // 进行类型擦除, 将 packaged_task 封装为 void() 类型的 lambda 并加入任务队列
        // 使得任务队列能够存储不同返回类型的函数, 提高了通用性
        m_task_queue.emplace([task_ptr]() { (*task_ptr)(); });
      }

      m_task_queue_cv.notify_one();

      return future;
    }
  };

  // 定义工作线程类
  class ThreadPool::WorkerThread
  {
   public:
    // 禁用拷贝/移动构造函数及赋值运算符
    WorkerThread(const WorkerThread&) = delete;
    WorkerThread(WorkerThread&&) = delete;
    WorkerThread& operator=(const WorkerThread&) = delete;
    WorkerThread& operator=(WorkerThread&&) = delete;

    /**
     * @brief 构造函数函数,
     * @details 新线程的 idle 时间一出生就算作已满 keep_alive_time, 如果它
     *          启动后马上没任务,下一轮监控就能立刻判断它符合缩容条件
     */
    explicit WorkerThread(ThreadPool* pool_ptr);

    /**
     * @brief 析构函数
     * @details 如果线程还在执行任务, 会等待线程执行完任务才销毁
     */
    ~WorkerThread();

    /**
     * @brief 终止当前工作线程
     */
    void terminate();

    /**
     * @brief 让线程进入暂停态
     * @details 只有当线程处于阻塞或运行态时会进入暂停态, 其他状态直接return不进行处理
     */
    void pause();

    /**
     * @brief 让线程从暂停态恢复至运行态
     * @details 只有当线程处于暂停态时, 会让其状态恢复至运行态; 其他状态不进行任何处理,
     *          直接return
     */
    void resume();

    /**
     * @brief 获取当前工作线程最后一次执行任务的时间
     */
    [[nodiscard]] std::chrono::steady_clock::time_point getLastActiveTime() const;

   private:
    enum class Status : std::uint8_t
    {
      RUNNING,      // 线程正在运行
      PAUSED,       // 线程被暂停
      TERMINATING,  // 线程将终止
      TERMINATED    // 线程已终止
    };

    ThreadPool* m_pool_ptr;
    std::atomic<Status> m_status;
    std::shared_mutex m_status_mutex;
    BinarySemaphore m_pause_sem;
    std::thread m_thread;
    std::atomic<std::chrono::steady_clock::time_point> m_last_active_time;

    /**
     * @brief 判断是否唤醒等待任务的线程
     * @details 当线程处于非运行态 或 线程池处于将终止态 或 任务队列非空时唤醒线程
     */
    bool isWake();

    /**
     * @brief 工作线程执行逻辑
     */
    void run();
  };
}  // namespace velox::threadpool