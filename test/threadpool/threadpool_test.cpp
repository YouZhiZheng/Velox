/**
 * @file threadpool_test.cpp
 * @brief 线程池模块测试
 * @author zhengyouzhi
 * @email youzhizheng9@gmail.com
 * @date 2025-7-31
 * @copyright Copyright (c) 2025 zhengyouzhi
 * @license   GNU General Public License v3.0
 *            See LICENSE file in the project root for full license information.
 */

#include "threadpool.hpp"

#include <gtest/gtest.h>

#include <future>
#include <iostream>
#include <stdexcept>
#include <vector>

#include "config.hpp"

using namespace velox::threadpool;

class ThreadPoolTest : public ::testing::Test
{
 protected:
  void SetUp() override
  {
    // 初始化日志器
    bool init_result = VELOX_LOG_INIT();
    ASSERT_TRUE(init_result) << "Failed to initialize logger!";
    pool_config.enable_dynamic_scaling = false;  // 关闭动态调整功能
  }

  void TearDown() override { VELOX_LOG_SHUTDOWN(); }

  ThreadPoolConfig pool_config;
};

// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)

// =================================================================================
// 1. 基本功能测试 (Basic Functionality Tests)
// =================================================================================
TEST_F(ThreadPoolTest, constructorAndDestructor)
{
  for (std::size_t k = 0; k < 10; ++k)
  {
    SCOPED_TRACE("Iteration: " + std::to_string(k));  // 提示是哪一轮

    pool_config.core_thread_count = 8;
    pool_config.max_task_count = 100;
    std::unique_ptr<ThreadPool> pool = std::make_unique<ThreadPool>(pool_config);
    EXPECT_EQ(pool->getThreadCount(), 8);
    EXPECT_EQ(pool->getStatus(), "RUNNING");

    pool->shutdown();
    EXPECT_EQ(pool->getStatus(), "TERMINATED");
    EXPECT_EQ(pool->getThreadCount(), 0);
  }
}

TEST_F(ThreadPoolTest, submitAndExecuteTask)
{
  for (std::size_t k = 0; k < 10; ++k)
  {
    SCOPED_TRACE("Iteration: " + std::to_string(k));  // 提示是哪一轮

    std::unique_ptr<ThreadPool> pool = std::make_unique<ThreadPool>(pool_config);
    std::atomic<int> counter(0);

    auto future = pool->submit(
        [&]()
        {
          counter.fetch_add(1);
          return "test";
        });

    EXPECT_EQ(future.get(), "test");
    EXPECT_EQ(counter.load(), 1);
  }
}

TEST_F(ThreadPoolTest, submitMultipleTasks)
{
  for (std::size_t k = 0; k < 10; ++k)
  {
    SCOPED_TRACE("Iteration: " + std::to_string(k));  // 提示是哪一轮

    pool_config.core_thread_count = 4;
    std::unique_ptr<ThreadPool> pool = std::make_unique<ThreadPool>(pool_config);
    std::atomic<std::size_t> counter(0);
    const std::size_t num_tasks = 100;

    std::vector<std::future<void>> futures;
    futures.reserve(num_tasks);

    for (std::size_t i = 0; i < num_tasks; ++i)
    {
      futures.push_back(pool->submit([&]() { counter.fetch_add(1); }));
    }

    for (auto& f : futures)
    {
      f.get();  // 等待所有任务完成
    }

    EXPECT_EQ(counter.load(), num_tasks);
  }
}

TEST_F(ThreadPoolTest, submitWhenPAUSED)
{
  for (std::size_t k = 0; k < 10; ++k)
  {
    SCOPED_TRACE("Iteration: " + std::to_string(k));  // 提示是哪一轮

    std::unique_ptr<ThreadPool> pool = std::make_unique<ThreadPool>(pool_config);
    std::atomic<std::size_t> counter(0);

    pool->pause();  // 暂停
    EXPECT_EQ(pool->getStatus(), "PAUSED");

    auto future = pool->submit([&]() { counter.fetch_add(1); });  // 提交任务
    std::this_thread::sleep_for(std::chrono::milliseconds(50));   // 等待一段时间

    EXPECT_EQ(counter.load(), 0);
  }
}

TEST_F(ThreadPoolTest, submitWhenQueueFull)
{
  for (std::size_t k = 0; k < 10; ++k)
  {
    SCOPED_TRACE("Iteration: " + std::to_string(k));  // 提示是哪一轮

    pool_config.max_task_count = 2;
    std::unique_ptr<ThreadPool> pool = std::make_unique<ThreadPool>(pool_config);

    // 控制第一个任务阻塞住工作线程，不让它完成
    std::promise<void> blocker;
    std::shared_future<void> blocker_future = blocker.get_future();
    pool->submit(
        [&blocker_future]()
        {
          // 阻塞等待手动释放
          blocker_future.wait();
        });

    // 等待任务被取走
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // 在提交两个任务将任务队列填充满
    pool->submit([]() {});
    pool->submit([]() {});

    // 任务队列满, 再次提交任务会抛出异常
    bool got_exception = false;
    try
    {
      pool->submit([]() {});
    }
    catch (const std::runtime_error& e)
    {
      got_exception = true;
    }

    EXPECT_TRUE(got_exception);

    // 释放阻塞任务, 避免线程池卡死
    blocker.set_value();
  }
}

TEST_F(ThreadPoolTest, submitWhenTERMINATED)
{
  for (std::size_t k = 0; k < 10; ++k)
  {
    SCOPED_TRACE("Iteration: " + std::to_string(k));  // 提示是哪一轮

    std::unique_ptr<ThreadPool> pool = std::make_unique<ThreadPool>(pool_config);
    pool->shutdown();
    EXPECT_EQ(pool->getStatus(), "TERMINATED");
    EXPECT_THROW({ pool->submit([]() {}); }, std::runtime_error);
  }
}

// =================================================================================
// 2. 状态转换测试 (State Transition Tests)
// =================================================================================
TEST_F(ThreadPoolTest, pausedAndResume)
{
  for (std::size_t k = 0; k < 10; ++k)
  {
    SCOPED_TRACE("Iteration: " + std::to_string(k));  // 提示是哪一轮

    pool_config.core_thread_count = 2;
    std::unique_ptr<ThreadPool> pool = std::make_unique<ThreadPool>(pool_config);
    std::atomic<int> counter(0);

    // 提交一个长时间任务
    auto long_task_future = pool->submit([]() { std::this_thread::sleep_for(std::chrono::milliseconds(3000)); });

    // 立即暂停
    pool->pause();
    EXPECT_EQ(pool->getStatus(), "PAUSED");

    // 在暂停状态下提交任务
    auto future_after_pause = pool->submit([&]() { counter.store(1); });

    // 即使有空闲的线程, 任务也不会执行
    std::this_thread::sleep_for(std::chrono::milliseconds(500));  // 等待一段时间
    EXPECT_EQ(counter.load(), 0);                                 // 判断任务是否执行

    // 恢复线程池
    pool->resume();
    EXPECT_EQ(pool->getStatus(), "RUNNING");

    // 等待任务执行
    future_after_pause.get();
    EXPECT_EQ(counter.load(), 1);
  }
}

TEST_F(ThreadPoolTest, pausedAndShutdown)
{
  for (std::size_t k = 0; k < 10; ++k)
  {
    SCOPED_TRACE("Iteration: " + std::to_string(k));  // 提示是哪一轮

    pool_config.core_thread_count = 2;
    std::unique_ptr<ThreadPool> pool = std::make_unique<ThreadPool>(pool_config);
    std::atomic<std::size_t> counter(0);
    const std::size_t num_tasks = 100;

    pool->pause();
    EXPECT_EQ(pool->getStatus(), "PAUSED");

    // 在暂停态提交任务
    for (std::size_t i = 0; i < num_tasks; ++i)
    {
      pool->submit([&]() { counter++; });
    }

    // 会先恢复至 RUNNING 再关闭
    pool->shutdown();
    EXPECT_EQ(counter.load(), num_tasks);
    EXPECT_EQ(pool->getStatus(), "TERMINATED");
    EXPECT_EQ(pool->getThreadCount(), 0);
  }
}

// =================================================================================
// 3. 动态调整测试 (Dynamic Resizing Tests)
// =================================================================================
TEST_F(ThreadPoolTest, increaseThreadCount)
{
  for (std::size_t k = 0; k < 10; ++k)
  {
    SCOPED_TRACE("Iteration: " + std::to_string(k));  // 提示是哪一轮

    pool_config.core_thread_count = 2;
    std::unique_ptr<ThreadPool> pool = std::make_unique<ThreadPool>(pool_config);
    EXPECT_EQ(pool->getThreadCount(), 2);

    // 在 RUNNING 态添加
    {
      EXPECT_EQ(pool->getStatus(), "RUNNING");
      pool->increaseThreadCount(2);
      EXPECT_EQ(pool->getThreadCount(), 4);
    }

    // 在 PAUSED 态添加
    {
      pool->pause();
      EXPECT_EQ(pool->getStatus(), "PAUSED");

      pool->increaseThreadCount(2);
      EXPECT_EQ(pool->getThreadCount(), 6);
    }

    // 在非法状态添加
    {
      pool->shutdown();
      EXPECT_EQ(pool->getStatus(), "TERMINATED");

      bool got_exception = false;
      try
      {
        pool->increaseThreadCount(2);
      }
      catch (const std::logic_error& e)
      {
        got_exception = true;
      }

      EXPECT_TRUE(got_exception);
    }
  }
}

TEST_F(ThreadPoolTest, decreaseThreadCount)
{
  for (std::size_t k = 0; k < 10; ++k)
  {
    SCOPED_TRACE("Iteration: " + std::to_string(k));  // 提示是哪一轮

    pool_config.core_thread_count = 6;
    std::unique_ptr<ThreadPool> pool = std::make_unique<ThreadPool>(pool_config);
    EXPECT_EQ(pool->getThreadCount(), 6);

    // 在 RUNNING 态删除
    {
      EXPECT_EQ(pool->getStatus(), "RUNNING");
      pool->decreaseThreadCount(2);
      EXPECT_EQ(pool->getThreadCount(), 4);
    }

    // 在 PAUSED 态删除
    {
      pool->pause();
      EXPECT_EQ(pool->getStatus(), "PAUSED");

      pool->decreaseThreadCount(2);
      EXPECT_EQ(pool->getThreadCount(), 2);
    }

    // 在非法状态删除
    {
      pool->shutdown();
      EXPECT_EQ(pool->getStatus(), "TERMINATED");

      bool got_exception = false;
      try
      {
        pool->decreaseThreadCount(2);
      }
      catch (const std::logic_error& e)
      {
        got_exception = true;
      }

      EXPECT_TRUE(got_exception);
    }
  }
}

TEST_F(ThreadPoolTest, decreaseThreadCountToZero)
{
  for (std::size_t k = 0; k < 10; ++k)
  {
    SCOPED_TRACE("Iteration: " + std::to_string(k));  // 提示是哪一轮

    pool_config.core_thread_count = 2;
    std::unique_ptr<ThreadPool> pool = std::make_unique<ThreadPool>(pool_config);
    pool->decreaseThreadCount(2);
    EXPECT_EQ(pool->getThreadCount(), 0);

    // 再删除线程, 测试健壮性
    pool->decreaseThreadCount(2);
    EXPECT_EQ(pool->getThreadCount(), 0);

    // 提交任务, 应该不会被执行直到有新线程加入
    std::atomic<bool> task_executed = false;
    auto f = pool->submit([&] { task_executed.store(true); });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));  // 等待一段时间
    EXPECT_FALSE(task_executed);

    pool->increaseThreadCount(1);
    f.get();
    EXPECT_TRUE(task_executed);
  }
}

// =================================================================================
// 4. 并发与压力测试 (Concurrency and Stress Tests)
// =================================================================================
TEST_F(ThreadPoolTest, concurrentSubmit)
{
  // 重复测试5次
  for (std::size_t k = 0; k < 5; ++k)
  {
    SCOPED_TRACE("Iteration: " + std::to_string(k));  // 提示是哪一轮

    pool_config.core_thread_count = 8;
    std::unique_ptr<ThreadPool> pool = std::make_unique<ThreadPool>(pool_config);
    std::atomic<int> counter(0);
    const std::size_t num_threads = 100;
    const std::size_t tasks_per_thread = 10000;

    std::vector<std::thread> submitter_threads;
    submitter_threads.reserve(num_threads);
    for (std::size_t i = 0; i < num_threads; ++i)
    {
      submitter_threads.emplace_back(
          [&]()
          {
            for (std::size_t j = 0; j < tasks_per_thread; ++j)
            {
              pool->submit([&]() { counter.fetch_add(1); });
            }
          });
    }

    // 等待每个线程提交结束
    for (auto& t : submitter_threads)
    {
      t.join();
    }

    // 优雅关闭, 确保所有任务都执行完毕
    pool->shutdown();

    EXPECT_EQ(counter.load(), num_threads * tasks_per_thread);
  }
}

// =================================================================================
// 5. 任务队列管理测试 (Task Queue Management Tests)
// =================================================================================
TEST_F(ThreadPoolTest, maxTaskCount)
{
  // 重复测试10次
  for (std::size_t k = 0; k < 10; ++k)
  {
    SCOPED_TRACE("Iteration: " + std::to_string(k));  // 提示是哪一轮

    // 只给一个工作线程, 便于模拟任务队列满
    pool_config.max_task_count = 6;
    std::unique_ptr<ThreadPool> pool = std::make_unique<ThreadPool>(pool_config);

    // 控制第一个任务阻塞住工作线程，不让它完成
    std::promise<void> blocker;
    std::shared_future<void> blocker_future = blocker.get_future();
    pool->submit(
        [&blocker_future]()
        {
          // 阻塞等待手动释放
          blocker_future.wait();
        });

    // 等待任务被取走
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // 提交任务直到队列满
    for (std::size_t i = 0; i < 6; ++i)
    {
      ASSERT_NO_THROW(pool->submit([] {}));
    }

    // 任务队列满, 再次提交任务会抛出异常
    bool got_exception = false;
    try
    {
      pool->submit([]() {});
    }
    catch (const std::runtime_error& e)
    {
      got_exception = true;
    }
    EXPECT_TRUE(got_exception);

    pool->setMaxTaskCount(10);
    // 再次提交任务, 队列满前不会抛出异常
    for (std::size_t i = 0; i < 4; ++i)
    {
      ASSERT_NO_THROW(pool->submit([] {}));
    }

    // 释放阻塞任务, 避免线程池卡死
    blocker.set_value();
  }
}

// =================================================================================
// 6. 边缘案例测试 (Edge Case Tests)
// =================================================================================
TEST_F(ThreadPoolTest, multiplePauseResumeCalls)
{
  // 重复测试10次
  for (std::size_t k = 0; k < 10; ++k)
  {
    SCOPED_TRACE("Iteration: " + std::to_string(k));  // 提示是哪一轮

    std::unique_ptr<ThreadPool> pool = std::make_unique<ThreadPool>(pool_config);

    pool->pause();
    EXPECT_EQ(pool->getStatus(), "PAUSED");
    // 多次调用
    for (std::size_t i = 0; i < 10; ++i)
    {
      ASSERT_NO_THROW(pool->pause());
    }

    std::atomic<bool> task_done = false;
    auto future = pool->submit([&] { task_done.store(true); });
    std::this_thread::sleep_for(std::chrono::milliseconds(50));  // 等待一段时间
    EXPECT_FALSE(task_done);

    pool->resume();
    EXPECT_EQ(pool->getStatus(), "RUNNING");
    // 多次调用
    for (std::size_t i = 0; i < 10; ++i)
    {
      ASSERT_NO_THROW(pool->resume());
    }

    future.get();
    EXPECT_TRUE(task_done);
  }
}

TEST_F(ThreadPoolTest, shutdownFromPausedState)
{
  // 重复测试10次
  for (std::size_t k = 0; k < 10; ++k)
  {
    SCOPED_TRACE("Iteration: " + std::to_string(k));  // 提示是哪一轮

    std::unique_ptr<ThreadPool> pool = std::make_unique<ThreadPool>(pool_config);
    std::atomic<int> counter = 0;

    pool->pause();
    ASSERT_EQ(pool->getStatus(), "PAUSED");

    for (std::size_t i = 0; i < 100; ++i)
    {
      pool->submit([&] { counter.fetch_add(1); });
    }

    pool->shutdown();
    ASSERT_EQ(pool->getStatus(), "TERMINATED");
    EXPECT_EQ(counter.load(), 100);

    // 多次调用测试
    for (std::size_t i = 0; i < 10; ++i)
    {
      ASSERT_NO_THROW(pool->shutdown());
    }
  }
}

TEST_F(ThreadPoolTest, submitTaskWithArgumentsAndReturnValue)
{
  // 重复测试10次
  for (std::size_t k = 0; k < 10; ++k)
  {
    SCOPED_TRACE("Iteration: " + std::to_string(k));  // 提示是哪一轮

    pool_config.core_thread_count = 4;
    std::unique_ptr<ThreadPool> pool = std::make_unique<ThreadPool>(pool_config);
    ASSERT_NE(pool, nullptr);

    // 1. lambda 值捕获
    {
      int x = 3;
      auto task = [x](int y) { return x + y; };
      auto future = pool->submit(task, 7);
      EXPECT_EQ(future.get(), 10);
    }

    // 2. lambda 引用捕获
    {
      std::string prefix = "Result:";
      auto task = [&prefix](int value) { return prefix + std::to_string(value); };
      auto future = pool->submit(task, 100);
      EXPECT_EQ(future.get(), "Result:100");
    }

    // 3. 普通函数
    {
      auto task = [](double a, double b) { return a * b; };
      auto future = pool->submit(task, 3.5, 2.0);
      EXPECT_DOUBLE_EQ(future.get(), 7.0);
    }

    // 4. 成员函数
    {
      struct MyClass
      {
        std::string test(int a, const std::string& b) { return b + std::to_string(a); }
      };

      MyClass obj;
      auto future = pool->submit(&MyClass::test, &obj, 99, "hello");
      EXPECT_EQ(future.get(), "hello99");
    }

    // 5. 静态成员函数
    {
      struct MyStatic
      {
        static auto makeString(char c, int count) { return std::string(count, c); }
      };
      auto future = pool->submit(&MyStatic::makeString, '#', 7);
      EXPECT_EQ(future.get(), "#######");
    }

    // 6. 函数对象(仿函数)
    {
      struct Functor
      {
        int operator()(int a, int b) const { return a - b; }
      };

      Functor functor;
      auto future = pool->submit(functor, 10, 3);
      EXPECT_EQ(future.get(), 7);
    }

    // 7. std::bind 绑定参数
    {
      auto add = [](int a, int b) { return a + b; };
      auto bound = std::bind(add, 1, 2);
      auto future = pool->submit(bound);
      EXPECT_EQ(future.get(), 3);
    }

    // 8. 返回 void 类型任务
    {
      std::atomic<int> count{ 0 };
      auto task = [&count](int delta) { count.fetch_add(delta); };
      auto future = pool->submit(task, 10);
      future.get();
      EXPECT_EQ(count.load(), 10);
    }

    // 9. 返回引用
    {
      auto task = [](std::string& s) -> std::string&
      {
        s += " modified";
        return s;
      };
      std::string data = "original";
      auto future = pool->submit(task, std::ref(data));  // 使用std::ref传引用
      std::string& ref_result = future.get();
      EXPECT_EQ(ref_result, "original modified");
      EXPECT_EQ(data, "original modified");
    }

    // 10. 抛异常的任务(确认异常传播)
    {
      auto task = []() { throw std::runtime_error("test"); };
      auto future = pool->submit(task);
      EXPECT_THROW(future.get(), std::runtime_error);
    }
  }
}

// =================================================================================
// 7. 自动线程数量调节测试 (Automatic Thread Number Adjustment Tests)
// =================================================================================
class ThreadPoolMonitorThreadTest : public ::testing::Test
{
 protected:
  void SetUp() override
  {
    // 初始化日志器
    bool init_result = VELOX_LOG_INIT();
    ASSERT_TRUE(init_result) << "Failed to initialize logger!";

    pool_config.core_thread_count = 2;
    pool_config.max_thread_count = 4;
    // 缩短时间, 便于测试
    pool_config.keep_alive_time = std::chrono::milliseconds(100);
    pool_config.monitor_interval = std::chrono::milliseconds(50);
  }

  void TearDown() override { VELOX_LOG_SHUTDOWN(); }

  ThreadPoolConfig pool_config;
};

TEST_F(ThreadPoolMonitorThreadTest, threadExpandsWhenAllThreadsBusy)
{
  for (std::size_t k = 0; k < 5; ++k)
  {
    SCOPED_TRACE("Iteration: " + std::to_string(k));  // 提示是哪一轮

    std::unique_ptr<ThreadPool> pool = std::make_unique<ThreadPool>(pool_config);
    ASSERT_EQ(pool->getThreadCount(), 2);

    // 创建两个阻塞任务, 模拟全部线程忙的情况
    std::promise<void> blocker1;
    std::shared_future<void> blocker1_future = blocker1.get_future();
    pool->submit(
        [&blocker1_future]()
        {
          // 阻塞等待手动释放
          blocker1_future.wait();
        });

    std::promise<void> blocker2;
    std::shared_future<void> blocker2_future = blocker2.get_future();
    pool->submit(
        [&blocker2_future]()
        {
          // 阻塞等待手动释放
          blocker2_future.wait();
        });

    // 再提交2个阻塞任务, 让新增的线程也忙
    std::promise<void> blocker3;
    std::shared_future<void> blocker3_future = blocker3.get_future();
    pool->submit(
        [&blocker3_future]()
        {
          // 阻塞等待手动释放
          blocker3_future.wait();
        });

    std::promise<void> blocker4;
    std::shared_future<void> blocker4_future = blocker4.get_future();
    pool->submit(
        [&blocker4_future]()
        {
          // 阻塞等待手动释放
          blocker4_future.wait();
        });

    // 提交一些任务, 模拟任务累积
    for (int i = 0; i < 3; i++)
    {
      pool->submit([]() {});
    }

    // 等待监控线程有机会扩容
    std::this_thread::sleep_for(pool_config.monitor_interval * 6);

    // 验证线程池已扩容
    EXPECT_EQ(pool->getThreadCount(), pool_config.max_thread_count);
    blocker1.set_value();
    blocker2.set_value();
    blocker3.set_value();
    blocker4.set_value();
  }
}

TEST_F(ThreadPoolMonitorThreadTest, threadShrinksWhenIdle)
{
  pool_config.core_thread_count = 1;
  pool_config.max_thread_count = 6;

  for (std::size_t k = 0; k < 5; ++k)
  {
    SCOPED_TRACE("Iteration: " + std::to_string(k));  // 提示是哪一轮

    std::unique_ptr<ThreadPool> pool = std::make_unique<ThreadPool>(pool_config);
    ASSERT_EQ(pool->getThreadCount(), 1);

    // 创建一个阻塞任务
    std::promise<void> blocker1;
    std::shared_future<void> blocker1_future = blocker1.get_future();
    pool->submit(
        [&blocker1_future]()
        {
          // 阻塞等待手动释放
          blocker1_future.wait();
        });

    // 提交 3 个任务, 使得任务累积
    for (int i = 0; i < 3; i++)
    {
      pool->submit([]() {});
    }

    // 等待扩容
    std::this_thread::sleep_for(pool_config.monitor_interval * 2);
    const std::size_t thread_count = pool->getThreadCount();
    EXPECT_GT(thread_count, pool_config.core_thread_count);
    EXPECT_LE(thread_count, pool_config.max_thread_count);

    // 释放阻塞任务
    blocker1.set_value();

    // 等待缩容
    std::this_thread::sleep_for(pool_config.monitor_interval * 10);

    EXPECT_EQ(pool->getThreadCount(), pool_config.core_thread_count);
  }
}

TEST_F(ThreadPoolMonitorThreadTest, keepAliveTimeForShrinking)
{
  for (std::size_t k = 0; k < 5; ++k)
  {
    SCOPED_TRACE("Iteration: " + std::to_string(k));  // 提示是哪一轮

    std::unique_ptr<ThreadPool> pool = std::make_unique<ThreadPool>(pool_config);
    ASSERT_EQ(pool->getThreadCount(), 2);

    pool->increaseThreadCount(1);  // 增加一个线程
    ASSERT_EQ(pool->getThreadCount(), 3);

    // 等待缩容
    std::this_thread::sleep_for(pool_config.monitor_interval * 2);
    EXPECT_EQ(pool->getThreadCount(), 2);

    // 等待超过keep_alive_time, 核心线程不会缩容
    std::this_thread::sleep_for(pool_config.keep_alive_time);
    EXPECT_EQ(pool->getThreadCount(), 2);
  }
}

TEST_F(ThreadPoolMonitorThreadTest, threadShrinksWhenPAUSED)
{
  for (std::size_t k = 0; k < 5; ++k)
  {
    SCOPED_TRACE("Iteration: " + std::to_string(k));  // 提示是哪一轮

    std::unique_ptr<ThreadPool> pool = std::make_unique<ThreadPool>(pool_config);
    ASSERT_EQ(pool->getThreadCount(), 2);

    pool->increaseThreadCount(2);
    ASSERT_EQ(pool->getThreadCount(), 4);

    pool->pause();
    // 提交2个任务, 使得任务累积
    for (int i = 0; i < 2; i++)
    {
      pool->submit([]() {});
    }

    // PAUSED态只会缩容, 等待缩容
    std::this_thread::sleep_for(pool_config.monitor_interval * 4);
    EXPECT_EQ(pool->getThreadCount(), 2);
  }
}

// =================================================================================
// 8. 控制文件更改配置测试 (Control File Change Configuration Tests)
// =================================================================================
class ThreadPoolConfigTest : public ::testing::Test
{
 protected:
  void SetUp() override
  {
    // 初始化日志器
    bool init_result = VELOX_LOG_INIT();
    ASSERT_TRUE(init_result) << "Failed to initialize logger!";

    pool = std::make_unique<ThreadPool>(pool_config);
    ASSERT_EQ(pool->getThreadCount(), 1);
  }

  void TearDown() override
  {
    if (pool != nullptr)
    {
      pool->shutdown();
    }

    VELOX_LOG_SHUTDOWN();
  }

  ThreadPoolConfig pool_config;
  std::unique_ptr<ThreadPool> pool;
};

TEST_F(ThreadPoolConfigTest, loadParametersFromYaml)
{
  auto pre_config = pool->getThreadPoolConfig();
  EXPECT_EQ(pre_config.max_task_count, 0);
  EXPECT_EQ(pre_config.core_thread_count, 1);
  EXPECT_EQ(pre_config.max_thread_count, 8);
  EXPECT_EQ(pre_config.keep_alive_time.count(), 5000);
  EXPECT_EQ(pre_config.monitor_interval.count(), 200);

  // 加载指定目录下的配置文件
  velox::config::Config::loadFromConfDir("test/threadpool");

  auto after_config = pool->getThreadPoolConfig();
  EXPECT_EQ(after_config.max_task_count, 1000);
  EXPECT_EQ(after_config.core_thread_count, 6);
  EXPECT_EQ(after_config.max_thread_count, 12);
  EXPECT_EQ(after_config.keep_alive_time.count(), 6000);
  EXPECT_EQ(after_config.monitor_interval.count(), 300);
}

// NOLINTEND(cppcoreguidelines-avoid-magic-numbers, readability-magic-numbers)
