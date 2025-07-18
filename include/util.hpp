
/**
 * @file util.hpp
 * @brief 常用的工具函数集合
 * @author zhengyouzhi
 * @email youzhizheng9@gmail.com
 * @date 2025-7-14
 * @copyright Copyright (c) 2025 zhengyouzhi
 * @license   GNU General Public License v3.0
 *            See LICENSE file in the project root for full license information.
 */

#pragma once

#include <cxxabi.h>

#include <filesystem>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <typeinfo>
#include <vector>

#ifndef PROJECT_ROOT_DIR
#define PROJECT_ROOT_DIR ""
#endif

namespace velox::util
{
  inline constexpr std::string_view k_project_root_dir = []()
  {
    // 编译时检查, 确保 PROJECT_ROOT_DIR 不为空
    static_assert(
        sizeof(PROJECT_ROOT_DIR) > 1,
        "PROJECT_ROOT_DIR is not defined correctly or is empty. Please check your CMake configuration.");

    return PROJECT_ROOT_DIR;
  }();

  /**
   * @brief 获取项目根路径
   */
  const std::filesystem::path& getProjectRootPath();

  /**
   * @brief 获取项目中指定子目录下所有指定后缀的文件(返回绝对路径)
   * @param[in] relativeDirPath 相对于项目根目录的目录路径, 例如 "conf"
   * @param[in] extension 要匹配的文件扩展名(如 ".yml", ".json")
   * @return 所有匹配的文件的绝对路径列表
   */
  std::vector<std::filesystem::path> listAllFilesByExt(
      const std::filesystem::path& relative_dir_path,
      const std::string& extension);

  /**
   * @brief 获取类型名称
   */
  template<typename T>
  const char* typeName()
  {
    static const std::string name = []
    {
      const char* raw = typeid(T).name();
      int status = 0;

      // RAII：用 unique_ptr 管理 malloc 得到的指针
      std::unique_ptr<char, void (*)(void*)> demangled(
          abi::__cxa_demangle(raw, nullptr, nullptr, &status),
          std::free  // 自定义 deleter：自动调用 free()
      );

      if (status == 0 && demangled)
      {
        return std::string(demangled.get());
      }

      return std::string(raw);
    }();

    return name.c_str();
  }

  /**
   * @brief 类型转化, 将原类型 F 转化成目标类型 T
   */
  template<typename T, typename F>
  T convert(const F& from_value)
  {
    std::stringstream ss;
    ss << from_value;
    T to_value{};
    ss >> to_value;

    if (ss.fail() || !ss.eof())
    {
      throw std::runtime_error("convert failed: invalid format");
    }

    return to_value;
  }

  /**
   * @brief 判断名字是否合法
   * @details 组成名字的字符只能为 [0-9a-zA-Z_.]
   */
  bool isValidName(const std::string& name) noexcept;

  /**
   * @brief 将文件的最后修改时间(file_time_type)转换为 Unix 风格的时间戳
   */
  uint64_t toUnixTimestamp(const std::filesystem::file_time_type& t);

}  // namespace velox::util