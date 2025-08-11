# Velox

> 高性能 C++ 基础开发框架，内置日志系统、YAML 配置管理、可控线程池等常用功能，让开发者专注业务逻辑而无需重复造轮子。

---

## 目录

- [简介](#简介)
- [特性](#特性)
- [快速开始](#快速开始)
  - [依赖](#依赖)
  - [构建](#构建)
  - [示例代码](#示例代码)
- [模块介绍](#模块介绍)
- [配置说明](#配置说明)
- [性能测试](#性能测试)
  - [日志模块](#日志模块)
  - [线程池模块](#线程池模块)
- [项目结构](#项目结构)
- [贡献指南](#贡献指南)
- [License](#license)

## 简介<a id="简介"></a>

Velox 是一个轻量、高性能、模块化的 C++ 开发框架，适用于快速构建具备稳定基础功能的应用程序。  
它封装了常见的底层组件，包括：

- **日志系统**（基于 [spdlog](https://github.com/gabime/spdlog)）
- **YAML 配置解析与管理**（基于 [yaml-cpp](https://github.com/jbeder/yaml-cpp)）
- **高性能可控线程池**（支持核心线程、最大线程、任务队列限制、暂停与恢复）
- **单元测试系统**（基于 [gtest](https://github.com/google/googletest)）

Velox 的设计目标是 **零冗余、模块化、高扩展性**，让开发者能专注于业务代码。

## 特性<a id="特性"></a>

- **高性能日志系统**
  - 基于 `spdlog`，支持同步/异步模式
  - 支持 `printf` 风格与 `std::format` 风格
  - 自动获取调用位置（文件、函数、行号）
- **YAML 配置管理**
  - 基于 `yaml-cpp` 封装
  - 支持配置文件加载、访问、热更新
- **可控线程池**
  - 支持核心线程与最大线程
  - 支持任务队列大小限制
  - 支持暂停、恢复、优雅关闭
- **模块化设计**
  - 模块可独立使用或组合使用
  - 提供统一的初始化与管理入口
- **单元测试**
  - 基于 GoogleTest 覆盖各模块

## 快速开始<a id="快速开始"></a>

### 依赖<a id="依赖"></a>

- 支持 **C++17** 标准的编译器 (推荐clang 18.1.8)
- Boost >= 1.87.0
- CMake >= 3.15
- Make >= 4.3

### 构建<a id="构建"></a>

```bash
git clone --recursive git@github.com:YouZhiZheng/Velox.git
cd velox
mkdir build && cd build
cmake ..
make -j$(nproc)
```

如果配置好项目根目录下的`MakeFile`文件的下面内容：

```bash
C_COMPILER := 
CXX_COMPILER := 
LINKER := 
```

也可直接在项目根目录下使用命令 `make debug` 或 `make release` 进行构建。

### 示例代码<a id="示例代码"></a>

```cpp
#include "log.hpp"
#include "config.hpp"
#include "threadpool.hpp"

#include <string>

int main() 
{
    // 初始化日志系统
    VELOX_LOG_INIT();

    VELOX_INFO("Start test {}", 1);      // 输出 info 级别的日志
    VELOX_DEBUG("Start test {}", 2);  // 输出 debug 级别的日志

    // 定义线程池配置变量
    velox::threadpool::ThreadPoolConfig pool_config;
    pool_config.core_thread_count = 2;    // 定义核心线程数为 2, 默认为 1
    pool_config.max_thread_count = 4;    // 定义最大线程数为 4, 默认为 8

    // 获取线程池指针
    auto pool = std::make_unique<ThreadPool>(pool_config);
    
    // 加载 config 目录下的配置文件
    velox::config::Config::loadFromConfDir("config");

    // 往线程池提交任务
    std::string prefix = "Result:";
    auto task = [&prefix](int value) { return prefix + std::to_string(value); };
    auto future = pool->submit(task, 100);
    VELOX_INFO("task result: {}", future.get());

    // 关闭日志系统
    VELOX_LOG_SHUTDOWN();
    return 0;
}
```

**注意：** 自己新创建的头文件和实现文件 (包括测试文件) 需要在 `cmake/SourcesAndHeaders.cmake` 文件中进行添加才能正确构建。

## 模块介绍<a id="模块介绍"></a>

项目各个模块的详细介绍可以查阅[此文档](https://zyzhi.top/tags/velox%E6%A8%A1%E5%9D%97/)。

## 配置说明<a id="配置说明"></a>

日志系统、线程池的配置文件均放在[`config`目录](https://github.com/YouZhiZheng/Velox/tree/main/config)下，每个模块可选配置均在对应`.yml`文件中以注释的形式给出。

如果想要在自己编写的新模块中使用配置系统，主要分为四步：

1. 编写对应的配置结构体，重载 `==` 操作符
2. 确定`.yml`文件格式
3. 实现对应的类型转换偏特化
4. 在该模块的构造函数中向配置系统注册

具体示例可参考[此文档](https://zyzhi.top/posts/1235a2f/#%e5%81%8f%e7%89%b9%e5%8c%96)。

## 性能测试<a id="性能测试"></a>

各个模块的测试代码均放在[test目录](https://github.com/YouZhiZheng/Velox/tree/main/test)下。

### 日志模块

日志模块直接通过简单封装`spdlog`库实现，所以日志模块的性能直接引用[spdlog](https://github.com/gabime/spdlog#Benchmarks)库的结果。

以下是在 Ubuntu 64 位、Intel i7-4770 CPU@3.40 GHz 中完成的一些基准测试([benchmarks](https://github.com/gabime/spdlog/blob/v1.x/bench/bench.cpp)):

同步模式

```bash
[info] **************************************************************
[info] Single thread, 1,000,000 iterations
[info] **************************************************************
[info] basic_st         Elapsed: 0.17 secs        5,777,626/sec
[info] rotating_st      Elapsed: 0.18 secs        5,475,894/sec
[info] daily_st         Elapsed: 0.20 secs        5,062,659/sec
[info] empty_logger     Elapsed: 0.07 secs       14,127,300/sec
[info] **************************************************************
[info] C-string (400 bytes). Single thread, 1,000,000 iterations
[info] **************************************************************
[info] basic_st         Elapsed: 0.41 secs        2,412,483/sec
[info] rotating_st      Elapsed: 0.72 secs        1,389,196/sec
[info] daily_st         Elapsed: 0.42 secs        2,393,298/sec
[info] null_st          Elapsed: 0.04 secs       27,446,957/sec
[info] **************************************************************
[info] 10 threads, competing over the same logger object, 1,000,000 iterations
[info] **************************************************************
[info] basic_mt         Elapsed: 0.60 secs        1,659,613/sec
[info] rotating_mt      Elapsed: 0.62 secs        1,612,493/sec
[info] daily_mt         Elapsed: 0.61 secs        1,638,305/sec
[info] null_mt          Elapsed: 0.16 secs        6,272,758/sec
```

异步模式

```bash
[info] -------------------------------------------------
[info] Messages     : 1,000,000
[info] Threads      : 10
[info] Queue        : 8,192 slots
[info] Queue memory : 8,192 x 272 = 2,176 KB 
[info] -------------------------------------------------
[info] 
[info] *********************************
[info] Queue Overflow Policy: block
[info] *********************************
[info] Elapsed: 1.70784 secs     585,535/sec
[info] Elapsed: 1.69805 secs     588,910/sec
[info] Elapsed: 1.7026 secs      587,337/sec
[info] 
[info] *********************************
[info] Queue Overflow Policy: overrun
[info] *********************************
[info] Elapsed: 0.372816 secs    2,682,285/sec
[info] Elapsed: 0.379758 secs    2,633,255/sec
[info] Elapsed: 0.373532 secs    2,677,147/sec
```

### 线程池模块

TODO

## 项目结构<a id="项目结构"></a>

```bash
.
├── CMakeLists.txt
├── LICENSE
├── Makefile
├── README.md
├── cmake                       # cmake辅助配置文件目录
├── config                      # 配置文件目录
├── include                     # 头文件目录
├── logs                        # 日志文件目录
├── src                         # 模块实现目录
├── test                        # 模块测试目录
│   ├── config
│   ├── log
│   └── threadpool
└── thirdparty                  # 第三方库
    ├── googletest
    ├── spdlog
    └── yaml-cpp
```

## 贡献指南<a id="贡献指南"></a>

欢迎提交 [Issue](https://github.com/YouZhiZheng/Velox/issues) 或 Pull Request 以完善 Velox！

## License<a id="license"></a>

本项目使用 GPL-3.0  许可证，详见 [LICENSE](LICENSE) 文件。
