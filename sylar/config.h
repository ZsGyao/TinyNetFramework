//
// Created by zgys on 23-2-4.
//

#ifndef __SYLAR_CONFIG_H__
#define __SYLAR_CONFIG_H__

#include <memory>
#include <sstream>
#include <string>
#include <boost/lexical_cast.hpp>
#include <map>
#include <yaml-cpp/yaml.h>
#include <algorithm>
#include <vector>
#include <map>
#include <list>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include "log.h"
#include "util.h"

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

        virtual std::string getTypeName() const = 0;

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
                node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };
    // -------- vector end --------

    //  --------   list   ---------
    template<class T>
    class LexicalCast<std::string, std::list<T> > {
    public:
        std::list<T> operator()(const std::string &v) {
            YAML::Node node = YAML::Load(v);
            typename std::list<T> vec;
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
    class LexicalCast<std::list<T>, std::string> {
    public:
        std::string operator()(const std::list<T> &v) {
            YAML::Node node;
            for (auto &i: v) {
                node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };
    // -------- list end --------

    //  --------   set   ---------
    template<class T>
    class LexicalCast<std::string, std::set<T> > {
    public:
        std::set<T> operator()(const std::string &v) {
            YAML::Node node = YAML::Load(v);
            typename std::set<T> vec;
            std::stringstream ss;
            for (size_t i = 0; i < node.size(); i++) {
                ss.str("");
                ss << node[i];
                vec.insert(LexicalCast<std::string, T>()(ss.str()));
            }
            return vec;
        }
    };

    template<class T>
    class LexicalCast<std::set<T>, std::string> {
    public:
        std::string operator()(const std::set<T> &v) {
            YAML::Node node;
            for (auto &i: v) {
                node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };
    // -------- set end --------

    //  --------  unordered_set   ---------
    template<class T>
    class LexicalCast<std::string, std::unordered_set<T> > {
    public:
        std::unordered_set<T> operator()(const std::string &v) {
            YAML::Node node = YAML::Load(v);
            typename std::unordered_set<T> vec;
            std::stringstream ss;
            for (size_t i = 0; i < node.size(); i++) {
                ss.str("");
                ss << node[i];
                vec.insert(LexicalCast<std::string, T>()(ss.str()));
            }
            return vec;
        }
    };

    template<class T>
    class LexicalCast<std::unordered_set<T>, std::string> {
    public:
        std::string operator()(const std::unordered_set<T> &v) {
            YAML::Node node;
            for (auto &i: v) {
                node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };
    // -------- unordered_set end --------

    //  --------  map   ---------
    template<class T>
    class LexicalCast<std::string, std::map<std::string, T> > {
    public:
        std::map<std::string, T> operator()(const std::string &v) {
            YAML::Node node = YAML::Load(v);
            typename std::map<std::string, T> vec;
            std::stringstream ss;
            for (auto it = node.begin(); it != node.end(); it++) {
                ss.str("");
                ss << it->second;
                vec.insert(std::make_pair(it->first.Scalar(), LexicalCast<std::string, T>()(ss.str())));
            }
            return vec;
        }
    };

    template<class T>
    class LexicalCast<std::map<std::string, T>, std::string> {
    public:
        std::string operator()(const std::map<std::string, T> &v) {
            YAML::Node node;
            for (auto &i: v) {
                node[i.first] = YAML::Load(LexicalCast<T, std::string>()(i.second));
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };
    // -------- map end --------

    //  --------  unordered_map   ---------
    template<class T>
    class LexicalCast<std::string, std::unordered_map<std::string, T> > {
    public:
        std::unordered_map<std::string, T> operator()(const std::string &v) {
            YAML::Node node = YAML::Load(v);
            typename std::unordered_map<std::string, T> vec;
            std::stringstream ss;
            for (auto it = node.begin(); it != node.end(); it++) {
                ss.str("");
                ss << it->second;
                vec.insert(std::make_pair(it->first.Scalar(), LexicalCast<std::string, T>()(ss.str())));
            }
            return vec;
        }
    };

    template<class T>
    class LexicalCast<std::unordered_map<std::string, T>, std::string> {
    public:
        std::string operator()(const std::unordered_map<std::string, T> &v) {
            YAML::Node node;
            for (auto &i: v) {
                node[i.first] = YAML::Load(LexicalCast<T, std::string>()(i.second));
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };
    // -------- unordered_map end --------

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
        typedef std::function<void(const T &old_value, const T &new_value)> on_change_cb;

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

        void setValue(const T &v) {
            if (v == m_val) {
                return;
            }
            for (auto &i: m_cbs) {
                i.second(m_val, v);
            }
            m_val = v;
        }

        std::string getTypeName() const override { return typeid(T).name(); }

        void addListener(uint64_t key, on_change_cb cb) {
            m_cbs[key] = cb;
        }

        void delListener(uint64_t key) {
            m_cbs.erase(key);
        }

        void clearListener() {
            m_cbs.clear();
        }

        on_change_cb getListener(uint64_t key) {
            auto it = m_cbs.find(key);
            return it == m_cbs.end() ? nullptr : it->second;
        }

    private:
        T m_val;
        //变更回调函数组，uint_64 key， 要求唯一， 一般可以用hash
        std::map<uint64_t, on_change_cb> m_cbs;
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
            auto it = GetDatas().find(name);
            if (it != GetDatas().end()) {
                auto tmp = std::dynamic_pointer_cast<ConfigVar<T> >(it->second);
                if (tmp) {
                    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "Lookup name=" << name << " exists";
                    return tmp;
                } else {
                    SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "Lookup name=" << name << " exists but type not "
                                                      << typeid(T).name() << " real_type:" << it->second->getTypeName()
                                                      << " " << it->second->toString();
                    return nullptr;
                }
            }

            if (name.find_first_not_of("abcdefghijklmnopqrstuvwxyz._0123456789") != std::string::npos) {
                SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "Lookup name invalid" << name;
                throw std::invalid_argument(name);
            }

            typename ConfigVar<T>::ptr v(new ConfigVar<T>(name, default_value, description));
            GetDatas()[name] = v;
            return v;
        }

        template<class T>
        static typename ConfigVar<T>::ptr Lookup(const std::string &name) {
            auto it = GetDatas().find(name);
            if (it == GetDatas().end()) {
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

        /**
         * @brief 遍历配置模块里面所有配置项
         * @param[in] cb 配置项回调函数
         */
        static void Visit(std::function<void(ConfigVarBase::ptr)> cb);

    private:
        static ConfigVarMap &GetDatas() {
            static ConfigVarMap s_datas;
            return s_datas;
        }
    };

}

#endif //SYLAR_CONFIG_H
