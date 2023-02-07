//
// Created by zgys on 23-2-4.
//

#ifndef __SYLAR_CONFIG_H__
#define __SYLAR_CONFIG_H__

#include <memory>
#include <sstream>
#include <string>
#include <boost/lexical_cast.hpp>
#include "log.h"
#include <map>
#include <yaml-cpp/yaml.h>
#include <algorithm>
#include <vector>

namespace sylar {
    /**
     * @brief 配置变量的基类
     */
    class ConfigVarBase {
    public:
        typedef std::shared_ptr<ConfigVarBase> ptr;

        /**
         * @brief 构造函数
         * @param[in] name 配置参数名称[0-9a-z_.]
         * @param[in] description 配置参数描述
         */
        ConfigVarBase(const std::string &name,
                      const std::string &description = "")
                : m_name(name),
                  m_description(description) {
            std::transform(m_name.begin(), m_name.end(), m_name.begin(), ::tolower);
        }

        virtual ~ConfigVarBase() {}

        const std::string &getName() const { return m_name; }

        const std::string &getDescription() const { return m_description; }

        virtual std::string toString() = 0;

        virtual bool fromString(const std::string &val) = 0;

    protected:
        std::string m_name;         //配置参数的名称
        std::string m_description;  // 配置参数的描述
    };

    // F from_type,  T to_type
    template<class F, class T>
    class LexicalCast {
    public:
        T operator()(const F &v) {
            return boost::lexical_cast<T>(v);
        }
    };

    // -------- 常用容器偏特化 ----------
    //  --------   vector   ---------
    template<class T>
    class LexicalCast<std::string, std::vector<T> > {
    public:
        std::vector<T> operator()(const std::string &v) {
            YAML::Node node = YAML::Load(v);
            typename std::vector<T> vec;
            std::stringstream ss;
            for (size_t i = 0; i < node.size(); i++) {
                ss.str("");
                ss << node[i];
                vec.push_back(LexicalCast<std::string, T>()(ss.str()));
            }
            return vec;
        }
    };

    template<class T>
    class LexicalCast<std::vector<T>, std::string> {
    public:
        std::string operator()(const std::vector<T> &v) {
            YAML::Node node;
            for (auto &i: v) {
                node.push_back(YAML::Load(LexicalCast<T, std::string>() (i)));
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };
    // -------- vector end --------

    // ------------ end --------------

    /**
    * @brief 配置参数模板子类,保存对应类型的参数值
    * @details T 参数的具体类型
    *          FromStr 从std::string转换成T类型的仿函数
     *                T operator()(const std::string&)
    *          ToStr 从T转换成std::string的仿函数
     *                std::string operator()(const T&)
    *          std::string 为YAML格式的字符串
     *
     *          模板特例化，基本属性使用预先定义的仿函数
    */
    template<class T, class FromStr = LexicalCast<std::string, T>, class ToStr = LexicalCast<T, std::string> >
    class ConfigVar : public ConfigVarBase {
    public:
        typedef std::shared_ptr<ConfigVar> ptr;

        ConfigVar(const std::string &name,
                  const T &default_value,
                  const std::string &description = "")
                : ConfigVarBase(name, description),
                  m_val(default_value) {
        }

        std::string toString() override {
            try {
                //return boost::lexical_cast<std::string>(m_val);
                return ToStr()(m_val);
            } catch (std::exception &e) {
                SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "ConfigVar::toString exception"
                                                  << e.what() << " convert: " << typeid(m_val).name() << "to string";
            }
            return "";
        }

        bool fromString(const std::string &val) override {
            try {
                //m_val = boost::lexical_cast<T>(val);  //把string类型转换成我们的成员类型
                setValue(FromStr()(val));
            } catch (std::exception &e) {
                SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "ConfigVar::fromString exception"
                                                  << e.what() << " convert: string to " << typeid(m_val).name();
            }
            return false;
        }

        const T getValue() const { return m_val; }

        void setValue(const T &v) { m_val = v; }

    private:
        T m_val;
    };

    /**
     * @brief ConfigVar的管理类
     * @details 提供便捷的方法创建/访问ConfigVar
     */
    class Config {
    public:
        typedef std::map<std::string, ConfigVarBase::ptr> ConfigVarMap;

        /**
        * @brief 获取/创建对应参数名的配置参数
        * @param[in] name 配置参数名称
        * @param[in] default_value 参数默认值
        * @param[in] description 参数描述
        * @details 获取参数名为name的配置参数,如果存在直接返回
        *          如果不存在,创建参数配置并用default_value赋值
        * @return 返回对应的配置参数,如果参数名存在但是类型不匹配则返回nullptr
        * @exception 如果参数名包含非法字符[^0-9a-z_.] 抛出异常 std::invalid_argument
        */
        template<class T>
        static typename ConfigVar<T>::ptr Lookup(const std::string &name,
                                                 const T &default_value,
                                                 const std::string &description = "") {
            auto tmp = Lookup<T>(name);
            if (tmp) {
                SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "Lookup name=" << name << " exists";
                return tmp;
            }

            if (name.find_first_not_of("abcdefghijklmnopqrstuvwxyz._0123456789") != std::string::npos) {
                SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "Lookup name invalid" << name;
                throw std::invalid_argument(name);
            }

            typename ConfigVar<T>::ptr v(new ConfigVar<T>(name, default_value, description));
            s_datas[name] = v;
            return v;
        }

        template<class T>
        static typename ConfigVar<T>::ptr Lookup(const std::string &name) {
            auto it = s_datas.find(name);
            if (it == s_datas.end()) {
                return nullptr;
            }
            return std::dynamic_pointer_cast<ConfigVar<T> >(it->second);
        }

        /**
         * @brief 使用YAML::Node初始化配置模块
         */
        static void LoadFromYaml(const YAML::Node &root);

        /**
         * @brief 查找配置参数,返回配置参数的基类
         * @param[in] name 配置参数名称
         */
        static ConfigVarBase::ptr LookupBase(const std::string &name);

    private:
        static ConfigVarMap s_datas;
    };

}

#endif //SYLAR_CONFIG_H