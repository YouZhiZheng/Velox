#include "util.hpp"

#include <algorithm>

namespace velox::util
{
  const std::filesystem::path& getProjectRootPath()
  {
    static const std::filesystem::path root_path(k_project_root_dir);
    return root_path;
  }

  bool isValidName(const std::string& name) noexcept
  {
    if (name.empty())
    {
      return false;
    }

    return std::all_of(
        name.begin(),
        name.end(),
        [](char c) { return ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '.' || c == '_'); });
  }

  std::vector<std::filesystem::path> listAllFilesByExt(
      const std::filesystem::path& relative_dir_path,
      const std::string& extension)
  {
    std::vector<std::filesystem::path> result;

    std::filesystem::path absolute_path = getProjectRootPath() / relative_dir_path;  // 子目录的绝对路径

    if (!std::filesystem::exists(absolute_path) || !std::filesystem::is_directory(absolute_path))
    {
      return result;  // 如果路径不存在或不是目录，返回空
    }

    for (const auto& entry : std::filesystem::recursive_directory_iterator(absolute_path))
    {
      if (entry.is_regular_file() && entry.path().extension() == extension)
      {
        result.emplace_back(entry.path());
      }
    }

    return result;
  }

  uint64_t toUnixTimestamp(const std::filesystem::file_time_type& t)
  {
    return static_cast<uint64_t>(t.time_since_epoch().count());
  }
}  // namespace velox::util