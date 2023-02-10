#ifndef __SYLAR_LOG_H__
#define __SYLAR_LOG_H__

#include <string>
#include <stdint.h>
#include <memory>
#include <list>
#include <sstream>
#include <fstream>
#include <vector>
#include <stdarg.h>
#include <iostream>
#include <map>
#include "util.h"
#include "singleton.h"
#include <yaml-cpp/yaml.h>


#define SYLAR_LOG_ROOT() sylar::LoggerMgr::GetInstance() -> getRoot()
#define SYLAR_LOG_NAME(name) sylar::LoggerMgr::GetInstance() -> getLogger(name)

#define SYLAR_LOG_LEVEL(logger, level) \
    if(logger->getLevel() <= level) \
        sylar::LogEventWrap(sylar::LogEvent::ptr(new sylar::LogEvent(logger, level, __FILE__, __LINE__, 0, sylar::GetThreadId(), \
        sylar::GetFiberId(), time(0)))).getSS()

#define SYLAR_LOG_DEBUG(logger) SYLAR_LOG_LEVEL(logger, sylar::LogLevel::DEBUG)
#define SYLAR_LOG_INFO(logger) SYLAR_LOG_LEVEL(logger, sylar::LogLevel::INFO)
#define SYLAR_LOG_WARN(logger) SYLAR_LOG_LEVEL(logger, sylar::LogLevel::WARN)
#define SYLAR_LOG_ERROR(logger) SYLAR_LOG_LEVEL(logger, sylar::LogLevel::ERROR)
#define SYLAR_LOG_FATAL(logger) SYLAR_LOG_LEVEL(logger, sylar::LogLevel::FATAL)


#define SYLAR_LOG_FMT_LEVEL(logger, level, fmt, ...) \
    if(logger->getLevel() <= level)                 \
        sylar::LogEventWrap(sylar::LogEvent::ptr(new sylar::LogEvent(logger, level, __FILE__, __LINE__, 0, sylar::GetThreadId(), \
        sylar::GetFiberId(), time(0)))).getEvent()->format(fmt, __VA_ARGS__)

#define SYLAR_LOG_FMT_DEBUG(logger, fmt, ...) SYLAR_LOG_FMT_LEVEL(logger, sylar::LogLevel::DEBUG, fmt, __VA_ARGS__)
#define SYLAR_LOG_FMT_INFO(logger, fmt, ...) SYLAR_LOG_FMT_LEVEL(logger, sylar::LogLevel::INFO, fmt, __VA_ARGS__)
#define SYLAR_LOG_FMT_WARN(logger, fmt, ...) SYLAR_LOG_FMT_LEVEL(logger, sylar::LogLevel::WARN, fmt, __VA_ARGS__)
#define SYLAR_LOG_FMT_ERROR(logger, fmt, ...) SYLAR_LOG_FMT_LEVEL(logger, sylar::LogLevel::ERROR, fmt, __VA_ARGS__)
#define SYLAR_LOG_FMT_FATAL(logger, fmt, ...) SYLAR_LOG_FMT_LEVEL(logger, sylar::LogLevel::FATAL, fmt, __VA_ARGS__)


namespace sylar {

    class Logger;
    class LoggerManager;

    //日志级别
    class LogLevel {
    public:
        enum Level {
            UNKNOW = 0,
            DEBUG = 1,
            INFO = 2,
            WARN = 3,
            ERROR = 4,
            FATAL = 5
        };

        //将日志级别转成文本输出
        static const char *ToString(LogLevel::Level level);

        //将文本转换成日志级别
        static LogLevel::Level FromString(const std::string &str);
    };


    //日志事件        把日志的字段封装成LogEvent
    class LogEvent {
    public:
        typedef std::shared_ptr<LogEvent> ptr; //定义LogEvent类型的共享指针为 ptr
        LogEvent(std::shared_ptr<Logger> logger, LogLevel::Level level, const char *file, int32_t line, uint32_t elapse,
                 uint32_t threadId, uint32_t fiberId, uint64_t time);
        LogEvent(std::shared_ptr<Logger> logger, LogLevel::Level level, const char *file, int32_t line, uint32_t elapse,
                 uint32_t threadId, uint32_t fiberId, uint64_t time,
                 const std::string &thread_name);

        const char *getFile() const { return m_file; }

        int32_t getLine() const { return m_line; }

        uint32_t getElapse() const { return m_elapse; }

        uint32_t getThreadId() const { return m_threadId; }

        uint32_t getFiberId() const { return m_elapse; }

        uint64_t getTime() const { return m_time; }

        const std::string &getThreadName() const { return m_threadName; }

        std::shared_ptr<Logger> getLogger() const { return m_logger; }

        LogLevel::Level getLevel() const { return m_level; }

        std::string getContent() const { return m_ss.str(); }

        std::stringstream &getSS() { return m_ss; }

        /**
         * @brief 格式化写入日志内容
        */
        void format(const char *fmt, ...);

        /**
         * @brief 格式化写入日志内容
         */
        void format(const char *fmt, va_list al);

    private:
        const char *m_file = nullptr;     //文件名
        int32_t m_line = 0;               //行号
        uint32_t m_elapse = 0;            //程序启动到现在的毫秒数
        uint32_t m_threadId = 0;          //线程id
        uint32_t m_fiberId = 0;           //协程id
        uint64_t m_time = 0;              //时间戳
        std::string m_threadName = "unkonw_thread_name";         //线程名称
        std::stringstream m_ss;           //日志输出内容流
        std::shared_ptr<Logger> m_logger; //日志器
        LogLevel::Level m_level;          //日志等级
    };

    /**
     * @brief 日志事件包装器
    */
    class LogEventWrap {
    public:

        /**
         * @brief 构造函数
         * @param[in] e 日志事件
         */
        LogEventWrap(LogEvent::ptr e);

        /**
         * @brief 析构函数
         */
        ~LogEventWrap();

        /**
         * @brief 获取日志事件
         */
        LogEvent::ptr getEvent() const { return m_event; }

        /**
         * @brief 获取日志内容流
         */
        std::stringstream &getSS();

    private:
        /**
         * @brief 日志事件
         */
        LogEvent::ptr m_event;
    };

    //输出日志的格式
    class LogFormatter {
    public:
        typedef std::shared_ptr<LogFormatter> ptr;

        LogFormatter(const std::string &pattern);

        /**
         * @brief 返回格式化日志文本
         * @param[in] logger 日志器
         * @param[in] level 日志级别
         * @param[in] event 日志事件
        */
        std::string format(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event);

        std::ostream &
        format(std::ostream &ofs, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event);

    public:
        /**
         * @brief 日志内容项格式化
        */
        class FormatItem {
        public:
            typedef std::shared_ptr<FormatItem> ptr;

            FormatItem(const std::string &fmt = "") {}

            virtual ~FormatItem() {}

            /**
             * @brief 格式化日志到流
             * @param[in, out] os 日志输出流
             * @param[in] logger 日志器
             * @param[in] level 日志等级
             * @param[in] event 日志事件
            */
            virtual void
            format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) = 0;
        };

        /**
         * @brief 初始化,解析日志模板
        */
        void init();

        /**
          * @brief 是否有错误
        */
        bool isError() const { return m_error; }

    private:
        std::string m_pattern;
        std::vector<FormatItem::ptr> m_items;
        bool m_error = false;
    };

    //日志输出目标
    class LogAppender {
        friend class Logger;

    public:
        typedef std::shared_ptr<LogAppender> ptr;

        /*
         * 虚析构函数，有许多日志输出地，LogAppender作为基类，析构函数需要写为虚析构
         */
        virtual ~LogAppender() {}

        /**
         * @brief 写入日志
         * @param[in] logger 日志器
         * @param[in] level 日志级别
         * @param[in] event 日志事件
         */
        virtual void log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) = 0;

        /**
         * @brief 更改日志格式器
        */
        void setFormatter(LogFormatter::ptr val) { m_formatter = val; }

        LogFormatter::ptr getFormatter() const { return m_formatter; }

        /**
         * @brief 设置日志级别
        */
        void setLevel(LogLevel::Level val) { m_level = val; }

    protected:
        LogLevel::Level m_level = LogLevel::DEBUG;  // 日志级别
        bool m_hasFormatter = false;                // 是否有自己的日志格式器
        LogFormatter::ptr m_formatter;              // 日志格式器
    };


    //日志器
    class Logger : public std::enable_shared_from_this<Logger> {
        friend class LoggerManager;

    public:
        typedef std::shared_ptr<Logger> ptr; //定义Logger类型的共享指针为 ptr

        /**
         * @brief 构造函数
         * @param[in] name 日志器名称
        */
        Logger(const std::string &name = "root");

        /**
         * @brief 写日志
         * @param[in] level 日志级别
         * @param[in] event 日志事件
        */
        void log(LogLevel::Level level, LogEvent::ptr event);

        /*
         * 输出debug级别的日志
         */
        void debug(LogEvent::ptr event);

        /*
         * 输出info级别的日志
         */
        void info(LogEvent::ptr event);

        /*
         * 输出warn级别的日志
         */
        void warn(LogEvent::ptr event);

        /*
         * 输出 error级别的日志
         */
        void error(LogEvent::ptr event);

        /*
         * 输出fatal级别的日志
         */
        void fatal(LogEvent::ptr event);

        /*
         * 添加Appender
         */
        void addAppender(LogAppender::ptr appender);

        /*
         * 删除Appender
         */
        void delAppender(LogAppender::ptr appender);

        void clearAppender();

        /*
         * 获取日志级别
         */
        LogLevel::Level getLevel() const {
            return m_level;
        }

        /*
         * 设置日志级别
         */
        void setLevel(LogLevel::Level level) { m_level = level; };

        /**
         * @brief 返回日志名称
        */
        const std::string &getName() const { return m_name; }

        /**
         * @brief 设置日志格式器
        */
        void setFormatter(LogFormatter::ptr val);

        /**
         * @brief 设置日志格式模板
         */
        void setFormatter(const std::string &val);

        /**
         * @brief 获取日志格式器
         */
        LogFormatter::ptr getFormatter();

    private:
        std::string m_name;                       // 日志名称
        LogLevel::Level m_level;                  // 日志级别
        std::list<LogAppender::ptr> m_appenders;  // 日志目标集合
        LogFormatter::ptr m_formatter;            // 日志格式器
        Logger::ptr m_root;                       // 主日志器
    };

    //输出到标准输出,控制台的Appender
    class StdoutLogAppender : public LogAppender {
    public:
        typedef std::shared_ptr<StdoutLogAppender> ptr;

        void log(Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override; //父类中重载的方法

    };

    //定义输出到文件的Appender
    class FileLogAppender : public LogAppender {
    public:
        typedef std::shared_ptr<FileLogAppender> ptr;

        FileLogAppender(const std::string &filename);

        ~FileLogAppender() override;

        void log(Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override;

        /**
         * @brief 重新打开日志文件
         * @return 成功返回true
        */
        bool reopen();

    private:
        std::string m_filename;       // 文件路径
        std::ofstream m_filestream;   // 文件流
        uint64_t m_lastTime = 0;      // 上次重新打开时间
    };

    /**
     * @brief 日志器管理类
    */
    class LoggerManager {
    public:
        /**
         * @brief 构造函数
         */
        LoggerManager();

        /**
         * @brief 获取日志器
         * @param[in] name 日志器名称
         */
        Logger::ptr getLogger(const std::string &name);

        /**
         * @brief 初始化
         */
        void init();

        /**
         * @brief 返回主日志器
         */
        Logger::ptr getRoot() const { return m_root; }

    private:
        // 日志器容器
        std::map<std::string, Logger::ptr> m_loggers;
        // 主日志器
        Logger::ptr m_root;
    };

// 日志器管理类单例模式
    typedef sylar::Singleton<LoggerManager> LoggerMgr;

}

#endif
