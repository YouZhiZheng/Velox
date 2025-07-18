/**
 * @file config.hpp
 * @brief 配置模块
 * @author zhengyouzhi
 * @email youzhizheng9@gmail.com
 * @date 2025-7-9
 * @copyright Copyright (c) 2025 zhengyouzhi
 * @license   GNU General Public License v3.0
 *            See LICENSE file in the project root for full license information.
 */

#pragma once

#include <yaml-cpp/yaml.h>

#include <list>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "log.hpp"
#include "util.hpp"

namespace velox::config
{
  /**
   * @brief 配置变量的基类
   */
  class ConfigVarBase
  {
   public:
    // 删除不需要的函数和操作
    ConfigVarBase(const ConfigVarBase&) = delete;
    ConfigVarBase& operator=(const ConfigVarBase&) = delete;
    ConfigVarBase(ConfigVarBase&&) = delete;
    ConfigVarBase& operator=(ConfigVarBase&&) = delete;

    using ptr = std::shared_ptr<ConfigVarBase>;

    /**
     * @brief 构造函数
     * @param[in] name 配置参数名称[0-9a-z_.]
     * @param[in] description 配置参数描述
     */
    explicit ConfigVarBase(std::string name, std::string description = "")
        : m_name(std::move(name)),
          m_description(std::move(description))
    {
    }

    /**
     * @brief 析构函数
     */
    virtual ~ConfigVarBase() = default;

    /**
     * @brief 返回配置参数名称
     */
    [[nodiscard]] const std::string& getName() const { return m_name; }

    /**
     * @brief 返回配置参数的描述
     */
    [[nodiscard]] const std::string& getDescription() const { return m_description; }

    /**
     * @brief 返回配置参数值的类型名称
     */
    [[nodiscard]] virtual std::string getTypeName() const = 0;

    /**
     * @brief 将配置参数的值序列化成一个字符串
     */
    [[nodiscard]] virtual std::string toString() = 0;

    /**
     * @brief 根据字符串的内容，反序列化回配置变量的内部值
     */
    virtual bool fromString(const std::string& val) = 0;

   private:
    std::string m_name;         // 配置参数的名称
    std::string m_description;  // 配置参数的描述
  };

  /**
   * @brief 类型转换模板类(F 源类型, T 目标类型)
   */
  template<typename F, typename T>
  class TypeConverter
  {
   public:
    /**
     * @brief 类型转换
     * @param[in] v 的源类型值
     * @return v 转换后的目标类型
     * @exception 当类型不可转换时抛出异常
     */
    T operator()(const F& v) { return velox::util::convert<T>(v); }
  };

  /**
   * @brief 类型转换模板类偏特化(将 YAML_tring 转化为 std::vector<T>)
   */
  template<typename T>
  class TypeConverter<std::string, std::vector<T>>
  {
   public:
    std::vector<T> operator()(const std::string& v)
    {
      YAML::Node node = YAML::Load(v);
      std::vector<T> vec;
      vec.reserve(node.size());  // 提前分配内存，提升性能
      std::stringstream ss;
      for (const auto& elem : node)
      {
        ss.str("");
        ss << elem;
        vec.push_back(TypeConverter<std::string, T>()(ss.str()));
      }

      return vec;
    }
  };

  /**
   * @brief 类型转换模板类偏特化(将 std::vector<T> 转化为 YAML_String )
   */
  template<typename T>
  class TypeConverter<std::vector<T>, std::string>
  {
   public:
    std::string operator()(const std::vector<T>& v)
    {
      YAML::Node node(YAML::NodeType::Sequence);
      for (const auto& i : v)
      {
        node.push_back(YAML::Load(TypeConverter<T, std::string>()(i)));
      }

      std::stringstream ss;
      ss << node;
      return ss.str();
    }
  };

  /**
   * @brief 类型转换模板类偏特化(将 YAML_tring 转化为 std::list<T>)
   */
  template<typename T>
  class TypeConverter<std::string, std::list<T>>
  {
   public:
    std::list<T> operator()(const std::string& v)
    {
      YAML::Node node = YAML::Load(v);
      std::list<T> ls;
      std::stringstream ss;
      for (const auto& elem : node)
      {
        ss.str("");
        ss << elem;
        ls.push_back(TypeConverter<std::string, T>()(ss.str()));
      }

      return ls;
    }
  };

  /**
   * @brief 类型转换模板类偏特化(将 std::list<T> 转化为 YAML_String )
   */
  template<typename T>
  class TypeConverter<std::list<T>, std::string>
  {
   public:
    std::string operator()(const std::list<T>& v)
    {
      YAML::Node node(YAML::NodeType::Sequence);
      for (const auto& i : v)
      {
        node.push_back(YAML::Load(TypeConverter<T, std::string>()(i)));
      }

      std::stringstream ss;
      ss << node;
      return ss.str();
    }
  };

  /**
   * @brief 类型转换模板类偏特化(将 YAML_tring 转化为 std::set<T>)
   */
  template<typename T>
  class TypeConverter<std::string, std::set<T>>
  {
   public:
    std::set<T> operator()(const std::string& v)
    {
      YAML::Node node = YAML::Load(v);
      std::set<T> s;
      std::stringstream ss;
      for (const auto& elem : node)
      {
        ss.str("");
        ss << elem;
        s.insert(TypeConverter<std::string, T>()(ss.str()));
      }

      return s;
    }
  };

  /**
   * @brief 类型转换模板类偏特化(将 std::set<T> 转化为 YAML_String )
   */
  template<typename T>
  class TypeConverter<std::set<T>, std::string>
  {
   public:
    std::string operator()(const std::set<T>& v)
    {
      YAML::Node node(YAML::NodeType::Sequence);
      for (const auto& i : v)
      {
        node.push_back(YAML::Load(TypeConverter<T, std::string>()(i)));
      }

      std::stringstream ss;
      ss << node;
      return ss.str();
    }
  };

  /**
   * @brief 类型转换模板类偏特化(将 YAML_tring 转化为 std::unordered_set<T>)
   */
  template<typename T>
  class TypeConverter<std::string, std::unordered_set<T>>
  {
   public:
    std::unordered_set<T> operator()(const std::string& v)
    {
      YAML::Node node = YAML::Load(v);
      std::unordered_set<T> us;
      std::stringstream ss;
      for (const auto& elem : node)
      {
        ss.str("");
        ss << elem;
        us.insert(TypeConverter<std::string, T>()(ss.str()));
      }

      return us;
    }
  };

  /**
   * @brief 类型转换模板类偏特化(将 std::unordered_set<T> 转化为 YAML_String )
   */
  template<typename T>
  class TypeConverter<std::unordered_set<T>, std::string>
  {
   public:
    std::string operator()(const std::unordered_set<T>& v)
    {
      YAML::Node node(YAML::NodeType::Sequence);
      for (const auto& i : v)
      {
        node.push_back(YAML::Load(TypeConverter<T, std::string>()(i)));
      }

      std::stringstream ss;
      ss << node;
      return ss.str();
    }
  };

  /**
   * @brief 类型转换模板类偏特化(将 YAML_tring 转化为 std::map<std::string, T>)
   */
  template<typename T>
  class TypeConverter<std::string, std::map<std::string, T>>
  {
   public:
    std::map<std::string, T> operator()(const std::string& v)
    {
      YAML::Node node = YAML::Load(v);
      std::map<std::string, T> mp;
      std::stringstream ss;
      for (auto it = node.begin(); it != node.end(); ++it)
      {
        ss.str("");
        ss << it->second;
        mp.insert(std::make_pair(it->first.Scalar(), TypeConverter<std::string, T>()(ss.str())));
      }

      return mp;
    }
  };

  /**
   * @brief 类型转换模板类偏特化(将 std::map<std::string, T> 转化为 YAML_String )
   */
  template<typename T>
  class TypeConverter<std::map<std::string, T>, std::string>
  {
   public:
    std::string operator()(const std::map<std::string, T>& v)
    {
      YAML::Node node(YAML::NodeType::Map);
      for (const auto& i : v)
      {
        node[i.first] = YAML::Load(TypeConverter<T, std::string>()(i.second));
      }

      std::stringstream ss;
      ss << node;
      return ss.str();
    }
  };

  /**
   * @brief 类型转换模板类偏特化(将 YAML_tring 转化为 std::unordered_map<std::string, T>)
   */
  template<typename T>
  class TypeConverter<std::string, std::unordered_map<std::string, T>>
  {
   public:
    std::unordered_map<std::string, T> operator()(const std::string& v)
    {
      YAML::Node node = YAML::Load(v);
      std::unordered_map<std::string, T> u_mp;
      std::stringstream ss;
      for (auto it = node.begin(); it != node.end(); ++it)
      {
        ss.str("");
        ss << it->second;
        u_mp.insert(std::make_pair(it->first.Scalar(), TypeConverter<std::string, T>()(ss.str())));
      }

      return u_mp;
    }
  };

  /**
   * @brief 类型转换模板类偏特化(将 std::unordered_map<std::string, T> 转化为 YAML_String )
   */
  template<typename T>
  class TypeConverter<std::unordered_map<std::string, T>, std::string>
  {
   public:
    std::string operator()(const std::unordered_map<std::string, T>& v)
    {
      YAML::Node node(YAML::NodeType::Map);
      for (const auto& i : v)
      {
        node[i.first] = YAML::Load(TypeConverter<T, std::string>()(i.second));
      }

      std::stringstream ss;
      ss << node;
      return ss.str();
    }
  };

  /**
   * @brief 配置参数模板子类,保存对应类型的参数值
   * @details T 参数的具体类型
   *          FromStr 从std::string转换成T类型的仿函数
   *          ToStr 从T转换成std::string的仿函数
   *          std::string 为YAML格式的字符串
   */
  template<typename T, typename FromStr = TypeConverter<std::string, T>, typename ToStr = TypeConverter<T, std::string>>
  class ConfigVar : public ConfigVarBase
  {
   public:
    using ptr = std::shared_ptr<ConfigVar>;
    using on_change_cb = std::function<void(const T& old_val, const T& new_val)>;  // 变更回调

    /**
     * @brief 通过参数名,参数值,描述构造ConfigVar
     * @param[in] name 参数名称有效字符为[0-9a-z_.]
     * @param[in] default_value 参数的默认值
     * @param[in] description 参数的描述
     */
    ConfigVar(const std::string& name, const T& default_value, const std::string& description = "")
        : ConfigVarBase(name, description),
          m_val(default_value)
    {
    }

    /**
     * @brief 返回参数值的类型名称(typeinfo)
     */
    [[nodiscard]] std::string getTypeName() const override { return velox::util::typeName<T>(); }

    /**
     * @brief 将参数值转换成 YAML String, 并返回
     * @exception 当转换失败抛出异常
     */
    [[nodiscard]] std::string toString() override
    {
      try
      {
        return ToStr()(m_val);
      }
      catch (std::exception& e)
      {
        VELOX_ERROR(
            "ConfigVar::toString failed. [Exception: '{}'] [Type: '{}'] [Config name: '{}']",
            e.what(),
            getTypeName(),
            getName());
      }
      return "";
    }

    /**
     * @brief 从YAML String 转成参数的值, 并进行赋值操作
     * @exception 当转换失败抛出异常
     */
    bool fromString(const std::string& val) override
    {
      try
      {
        setValue(FromStr()(val));
      }
      catch (std::exception& e)
      {
        VELOX_ERROR(
            "ConfigVar::fromString failed. [Exception: {}] [Failed to convert yaml-string {} to type {}] [Config "
            "name: {}]",
            e.what(),
            val,
            getTypeName(),
            getName());

        return false;
      }

      return true;
    }

    /**
     * @brief 获取当前参数的值
     */
    T getValue() const { return m_val; }

    /**
     * @brief 设置当前参数的值
     * @details 当参数的值发生变化时会调用该变量所对应的变更回调函数
     */
    void setValue(const T& value)
    {
      if (value == m_val)
      {
        return;
      }

      T old_val = m_val;
      m_val = value;

      for (const auto& [id, cb] : m_cbs)
      {
        cb(old_val, m_val);
      }
    }

    /**
     * @brief 添加变化回调函数
     * @return 返回该回调函数对应的唯一 id, 用于删除回调
     */
    uint64_t addListener(on_change_cb cb)
    {
      static uint64_t s_fun_id = 0;
      ++s_fun_id;
      m_cbs[s_fun_id] = cb;
      return s_fun_id;
    }

    /**
     * @brief 删除回调函数
     * @param[in] key 回调函数的唯一id
     */
    void delListener(uint64_t key) { m_cbs.erase(key); }

    /**
     * @brief 获取回调函数
     * @param[in] key 回调函数的唯一id
     * @return 如果存在返回对应的回调函数,否则返回 nullptr
     */
    on_change_cb getListener(uint64_t key)
    {
      auto it = m_cbs.find(key);
      return it == m_cbs.end() ? nullptr : it->second;
    }

    /**
     * @brief 清理所有的回调函数
     */
    void clearAllListener() { m_cbs.clear(); }

   private:
    T m_val;
    // 变更回调函数组, key为 uint64_t
    std::unordered_map<uint64_t, on_change_cb> m_cbs;
  };

  /**
   * @brief ConfigVar的管理类
   * @details 提供便捷的方法创建 /访问ConfigVar
   */
  class Config
  {
   public:
    using ConfigVarMap = std::unordered_map<std::string, ConfigVarBase::ptr>;

    /**
     * @brief 获取 /创建对应参数名的配置参数
     * @param[in] name 配置参数名称
     * @param[in] default_value 参数默认值
     * @param[in] description 参数描述
     * @details 获取参数名为name的配置参数,如果存在则直接返回
     *          如果不存在, 则根据参数创建对应的配置参数
     * @return 返回对应的配置参数指针, 如果参数名存在但是类型不匹配则返回 nullptr
     * @exception 如果参数名包含非法字符[^0-9a-z_.] 抛出异常 std::invalid_argument
     */
    template<class T>
    static typename ConfigVar<T>::ptr
    getOrCreateConfigVarPtr(const std::string& name, const T& default_value, const std::string& description = "")
    {
      auto it = getDatas().find(name);
      if (it != getDatas().end())
      {
        auto tmp = std::dynamic_pointer_cast<ConfigVar<T>>(it->second);
        if (tmp)
        {
          VELOX_INFO("ConfigVar [name:{}, valueType:{}] exists", name, tmp->getTypeName());
          return tmp;
        }

        // 存在但类型不匹配
        VELOX_ERROR(
            "ConfigVar name {} exists but type not {}, real type is {}:{}",
            name,
            velox::util::typeName<T>(),
            it->second->getTypeName(),
            it->second->toString());
        return nullptr;
      }

      // 创建配置参数
      if (!velox::util::isValidName(name))
      {
        VELOX_ERROR("ConfigVar name invalid: {}", name);
        throw std::invalid_argument(name);
      }

      auto var = std::make_shared<ConfigVar<T>>(name, default_value, description);
      getDatas()[name] = var;
      return var;
    }

    /**
     * @brief 获取配置参数
     * @param[in] name 配置参数名称
     * @return 返回配置参数名为name的配置参数指针, 不存在则返回 nullptr
     */
    template<class T>
    static typename ConfigVar<T>::ptr getConfigVarPtr(const std::string& name)
    {
      auto it = getDatas().find(name);
      if (it == getDatas().end())
      {
        return nullptr;
      }

      return std::dynamic_pointer_cast<ConfigVar<T>>(it->second);
    }

    /**
     * @brief 获取配置参数
     * @param[in] name 配置参数名称
     * @return 返回配置参数名为name的配置参数基类指针, 不存在则返回 nullptr
     */
    static ConfigVarBase::ptr getConfigVarBasePtr(const std::string& name);

    /**
     * @brief 使用 YAML::Node 更新对应配置参数
     * @details 只会更新已经存在的配置参数, 未存在的配置参数不会创建, 只会在日志中
     *          记录
     */
    static void loadFromYaml(const YAML::Node& root);

    /**
     * @brief 加载指定项目子目录里面的配置文件
     * @details 根据 force 参数来判断是否强制加载全部配置文件
     *          force == false时, 只加载更新过的配置文件
     */
    static void loadFromConfDir(const std::string& relative_dir_path, bool force = false);

    /**
     * @brief 清空所有已存储的数据
     */
    static void clearAllData() { getDatas().clear(); };

   private:
    /**
     * @brief 返回所有的配置项
     */
    static ConfigVarMap& getDatas()
    {
      static ConfigVarMap s_datas;
      return s_datas;
    }
  };

}  // namespace velox::config
