#include "log.h"
#include "config.h"


namespace sylar {

    const char *LogLevel::ToString(LogLevel::Level level) {
        switch (level) {
#define XX(name) \
            case LogLevel::name: \
            return #name; \
            break;

            XX(DEBUG);
            XX(INFO);
            XX(WARN);
            XX(ERROR);
            XX(FATAL);
#undef XX
            default:
                return "UNKNOW";
        }
        return "UNKONW";
    }

    LogLevel::Level LogLevel::FromString(const std::string &str) {
#define XX(level, v) \
    if(str == #v) {  \
     return LogLevel::level; \
     }
        XX(DEBUG, debug);
        XX(INFO, info);
        XX(WARN, warn);
        XX(ERROR, error);
        XX(FATAL, fatal);

        XX(DEBUG, DEBUG);
        XX(INFO, INFO);
        XX(WARN, WARN);
        XX(ERROR, ERROR);
        XX(FATAL, FATAL);
        return LogLevel::UNKNOW;
#undef XX
    }

    LogEventWrap::LogEventWrap(LogEvent::ptr e)
            : m_event(e) {
    }

    LogEventWrap::~LogEventWrap() {
        m_event->getLogger()->log(m_event->getLevel(), m_event);
    }

    std::stringstream &LogEventWrap::getSS() {
        return m_event->getSS();
    }

    LogEvent::LogEvent(std::shared_ptr<Logger> logger, LogLevel::Level level, const char *file, int32_t line,
                       uint32_t elapse,
                       uint32_t threadId, uint32_t fiberId, uint64_t time)

            : m_file(file),
              m_line(line),
              m_elapse(elapse),
              m_threadId(threadId),
              m_fiberId(fiberId),
              m_time(time),
              m_logger(logger),
              m_level(level) {

    }

    LogEvent::LogEvent(std::shared_ptr<Logger> logger, LogLevel::Level level, const char *file, int32_t line,
                       uint32_t elapse, uint32_t thread_id, uint32_t fiber_id, uint64_t time,
                       const std::string &thread_name)
            : m_file(file),
              m_line(line),
              m_elapse(elapse),
              m_threadId(thread_id),
              m_fiberId(fiber_id),
              m_time(time),
              m_threadName(thread_name),
              m_logger(logger),
              m_level(level) {
    }

    void LogEvent::format(const char *fmt, ...) {
        va_list al;
        va_start(al, fmt);
        format(fmt, al);
        va_end(al);
    }

    void LogEvent::format(const char *fmt, va_list al) {
        char *buf = nullptr;
        int len = vasprintf(&buf, fmt, al);
        if (len != -1) {
            m_ss << std::string(buf, len);
            free(buf);
        }
    }

    class MessageFormatItem : public LogFormatter::FormatItem {
    public:
        MessageFormatItem(const std::string &str = "") {}

        void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
            os << event->getContent();
        }
    };

    class LevelFormatItem : public LogFormatter::FormatItem {
    public:
        LevelFormatItem(const std::string &str = "") {}

        void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
            os << LogLevel::ToString(level);
        }
    };

    class ElapseFormatItem : public LogFormatter::FormatItem {
    public:
        ElapseFormatItem(const std::string &str = "") {}

        void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
            os << event->getElapse();
        }
    };

    class LogNameFormatItem : public LogFormatter::FormatItem {
    public:
        LogNameFormatItem(const std::string &str = "") {}

        void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
            os << event->getLogger()->getName();
        }
    };

    class ThreadIdFormatItem : public LogFormatter::FormatItem {
    public:
        ThreadIdFormatItem(const std::string &str = "") {}

        void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
            os << event->getThreadId();
        }
    };

    class ThreadNameFormatItem : public LogFormatter::FormatItem {
    public:
        ThreadNameFormatItem(const std::string &str = "") {}

        void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
            os << event->getThreadName();
        }
    };

    class StringFormatItem : public LogFormatter::FormatItem {
    public:
        StringFormatItem(const std::string &str)
                : LogFormatter::FormatItem(str), m_string(str) {}

        void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
            os << m_string;
        }

    private:
        std::string m_string;
    };

    class FiberIdFormatItem : public LogFormatter::FormatItem {
    public:
        FiberIdFormatItem(const std::string &str = "") {}

        void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
            os << event->getFiberId();
        }
    };

    class DateTimeFormatItem : public LogFormatter::FormatItem {
    public:
        DateTimeFormatItem(const std::string &format = "%Y-%m-%d %H:%M:%S")
                : m_format(format) {
            if (m_format.empty()) {
                m_format = "%Y-%m-%d %H:%M:%S";
            }
        }

        void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
            struct tm tm;
            time_t time = event->getTime();
            localtime_r(&time, &tm);
            char buf[64];
            strftime(buf, sizeof(buf), m_format.c_str(), &tm);
            os << buf;
        }

    private:
        std::string m_format;
    };

//-----------------------------------
    class FilenameFormatItem : public LogFormatter::FormatItem {
    public:
        FilenameFormatItem(const std::string &str = "") {}

        void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
            os << event->getFile();
        }
    };

    class LineFormatItem : public LogFormatter::FormatItem {
    public:
        LineFormatItem(const std::string &str = "") {}

        void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
            os << event->getLine();
        }
    };

    class NewLineFormatItem : public LogFormatter::FormatItem {
    public:
        NewLineFormatItem(const std::string &str = "") {}

        void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
            os << std::endl;
        }
    };

    class TabFormatItem : public LogFormatter::FormatItem {
    public:
        TabFormatItem(const std::string &str = "") {}

        void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override {
            os << "\t";
        }

    private:
        std::string m_string;
    };

    Logger::Logger(const std::string &name)
            : m_name(name), m_level(LogLevel::DEBUG) {
        //m_formatter.reset(new LogFormatter("%d{%Y-%m-%d %H:%M:%S}%T%t%T%N%T%F%T[%p]%T[%c]%T%f:%l%T%m%n"));
        m_formatter.reset(new LogFormatter("%d{%Y-%m-%d %H:%M:%S}%T%t%T%F%T[%p]%T[%c]%T%f:%l%T%m%n"));

    }

    void Logger::setFormatter(LogFormatter::ptr val) {
        //MutexType::Lock lock(m_mutex);
        m_formatter = val;

        for (auto &i: m_appenders) {
            //MutexType::Lock ll(i->m_mutex);
            if (!i->m_hasFormatter) {
                i->m_formatter = m_formatter;
            }
        }
    }

    void Logger::setFormatter(const std::string &val) {
        //std::cout << "---" << val << std::endl;
        sylar::LogFormatter::ptr new_val(new sylar::LogFormatter(val));
        if (new_val->isError()) {
            std::cout << "Logger setFormatter name=" << m_name
                      << " value=" << val << " invalid formatter"
                      << std::endl;
            return;
        }
        //m_formatter = new_val;
        setFormatter(new_val);
    }

    LogFormatter::ptr Logger::getFormatter() {
        //MutexType::Lock lock(m_mutex);
        return m_formatter;
    }

    void Logger::addAppender(LogAppender::ptr appender) {
        if (!appender->getFormatter()) {
            appender->setFormatter(m_formatter);
        }
        m_appenders.push_back(appender);
    }

    void Logger::delAppender(LogAppender::ptr appender) {
        for (auto it = m_appenders.begin(); it != m_appenders.end(); it++) {
            if (*it == appender) {
                m_appenders.erase(it);
                break;
            }
        }
    }

    void Logger::clearAppender() {
        m_appenders.clear();
    }

    //------------------------------
    void Logger::log(LogLevel::Level level, LogEvent::ptr event) {
        if (level >= m_level) {
            auto self = shared_from_this();
            if (!m_appenders.empty()) {
                for (auto &i: m_appenders) {
                    i->log(self, level, event);
                }
            } else if (m_root) {
                m_root->log(level, event);
            }
        }
    }

    void Logger::debug(LogEvent::ptr event) {
        log(LogLevel::DEBUG, event);
    }

    void Logger::info(LogEvent::ptr event) {
        log(LogLevel::INFO, event);
    }

    void Logger::warn(LogEvent::ptr event) {
        log(LogLevel::WARN, event);
    }

    void Logger::error(LogEvent::ptr event) {
        log(LogLevel::ERROR, event);
    }

    void Logger::fatal(LogEvent::ptr event) {
        log(LogLevel::FATAL, event);
    }

    std::string Logger::toYamlString() {
        //MutexType::Lock lock(m_mutex);
        YAML::Node node;
        node["name"] = m_name;
        if(m_level != LogLevel::UNKNOW) {
            node["level"] = LogLevel::ToString(m_level);
        }
        if(m_formatter) {
            node["formatter"] = m_formatter->getPattern();
        }

        for(auto& i : m_appenders) {
            node["appenders"].push_back(YAML::Load(i->toYamlString()));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }

    FileLogAppender::FileLogAppender(
            const std::string &filename)
            : m_filename(filename) {
        reopen();
    }

    FileLogAppender::~FileLogAppender() {
        if (m_filestream.is_open()) {
            m_filestream.close();
        }
    }

    void FileLogAppender::log(Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) {
        if (level >= m_level) {
            m_filestream << m_formatter->format(logger, level, event);
        }
    }

    bool FileLogAppender::reopen() {
        if (m_filestream.is_open()) {
            m_filestream.close();
        }
        m_filestream.open(m_filename);
        return !!m_filestream; //使用！！让m_filestream变量返回为bool型
    }

    std::string FileLogAppender::toYamlString() {
        //MutexType::Lock lock(m_mutex);
        YAML::Node node;
        node["type"] = "FileLogAppender";
        node["file"] = m_filename;
        if(m_level != LogLevel::UNKNOW) {
            node["level"] = LogLevel::ToString(m_level);
        }
        if(m_hasFormatter && m_formatter) {
            node["formatter"] = m_formatter->getPattern();
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }

    void StdoutLogAppender::log(Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) {
        if (level >= m_level) {
            std::cout << m_formatter->format(logger, level, event);
        }
    }

    std::string StdoutLogAppender::toYamlString() {
        //MutexType::Lock lock(m_mutex);
        YAML::Node node;
        node["type"] = "StdoutLogAppender";
        if(m_level != LogLevel::UNKNOW) {
            node["level"] = LogLevel::ToString(m_level);
        }
        if(m_hasFormatter && m_formatter) {
            node["formatter"] = m_formatter->getPattern();
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }

    LogFormatter::LogFormatter(
            const std::string &pattern)
            : m_pattern(pattern) {
        init();
    }

    std::string LogFormatter::format(Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) {
        std::stringstream ss;
        for (auto &i: m_items) {
            i->format(ss, logger, level, event);
        }
        return ss.str();
    }

    std::ostream &LogFormatter::format(std::ostream &ofs, std::shared_ptr<Logger> logger, LogLevel::Level level,
                                       LogEvent::ptr event) {
        for (auto &i: m_items) {
            i->format(ofs, logger, level, event);
        }
        return ofs;
    }


    /**
 * 简单的状态机判断，提取pattern中的常规字符和模式字符
 *
 * 解析的过程就是从头到尾遍历，根据状态标志决定当前字符是常规字符还是模式字符
 *
 * 一共有两种状态，即正在解析常规字符和正在解析模板转义字符
 *
 * 比较麻烦的是%%d，后面可以接一对大括号指定时间格式，比如%%d{%%Y-%%m-%%d %%H:%%M:%%S}，这个状态需要特殊处理
 *
 * 一旦状态出错就停止解析，并设置错误标志，未识别的pattern转义字符也算出错
 *
 * @see LogFormatter::LogFormatter
 */
    void LogFormatter::init() {
        // 按顺序存储解析到的pattern项
        // 每个pattern包括一个整数类型和一个字符串，类型为0表示该pattern是常规字符串，为1表示该pattern需要转义
        // 日期格式单独用下面的dataformat存储
        std::vector<std::pair<int, std::string>> patterns;
        // 临时存储常规字符串
        std::string tmp;
        // 日期格式字符串，默认把位于%d后面的大括号对里的全部字符都当作格式字符，不校验格式是否合法
        std::string dateformat;
        // 是否解析出错
        bool error = false;

        // 是否正在解析常规字符，初始时为true
        bool parsing_string = true;
        // 是否正在解析模板字符，%后面的是模板字符
        // bool parsing_pattern = false;

        size_t i = 0;
        while (i < m_pattern.size()) {
            std::string c = std::string(1, m_pattern[i]);
            if (c == "%") {
                if (parsing_string) {
                    if (!tmp.empty()) {
                        patterns.push_back(std::make_pair(0, tmp));
                    }
                    tmp.clear();
                    parsing_string = false; // 在解析常规字符时遇到%，表示开始解析模板字符
                    // parsing_pattern = true;
                    i++;
                    continue;
                } else {
                    patterns.push_back(std::make_pair(1, c));
                    parsing_string = true; // 在解析模板字符时遇到%，表示这里是一个%转义
                    // parsing_pattern = false;
                    i++;
                    continue;
                }
            } else { // not %
                if (parsing_string) { // 持续解析常规字符直到遇到%，解析出的字符串作为一个常规字符串加入patterns
                    tmp += c;
                    i++;
                    continue;
                } else { // 模板字符，直接添加到patterns中，添加完成后，状态变为解析常规字符，%d特殊处理
                    patterns.push_back(std::make_pair(1, c));
                    parsing_string = true;
                    // parsing_pattern = false;

                    // 后面是对%d的特殊处理，如果%d后面直接跟了一对大括号，那么把大括号里面的内容提取出来作为dateformat
                    if (c != "d") {
                        i++;
                        continue;
                    }
                    i++;
                    if (i < m_pattern.size() && m_pattern[i] != '{') {
                        continue;
                    }
                    i++;
                    while (i < m_pattern.size() && m_pattern[i] != '}') {
                        dateformat.push_back(m_pattern[i]);
                        i++;
                    }
                    if (m_pattern[i] != '}') {
                        // %d后面的大括号没有闭合，直接报错
                        std::cout << "[ERROR] LogFormatter::init() " << "pattern: [" << m_pattern << "] '{' not closed"
                                  << std::endl;
                        error = true;
                        break;
                    }
                    i++;
                    continue;
                }
            }
        } // end while(i < m_pattern.size())

        if (error) {
            m_error = true;
            return;
        }

        // 模板解析结束之后剩余的常规字符也要算进去
        if (!tmp.empty()) {
            patterns.push_back(std::make_pair(0, tmp));
            tmp.clear();
        }

        // for debug
//         std::cout << "patterns:" << std::endl;
//         for(auto &v : patterns) {
//             std::cout << "type = " << v.first << ", value = " << v.second << std::endl;
//         }
//         std::cout << "dataformat = " << dateformat << std::endl;

// %m -- 输出代码中指定的消息
        // %p -- 输出优先级 DEBUG, INFO, WARN, ERROR, FATAL
        // %r -- 输出自应用启动到输出该log信息耗费的毫秒数
        // %c -- 输出所属的类目，通常就是所在类的全名
        // %t -- 输出产生该日志事件的线程名
        // %n -- 输出一个回车换行符，Windows平台为“\\r\\n”， Unix平台为“\\n”
        // %d -- 输出日志时间点的日期或时间，比如 %d{yyy MMM dd HH:mm:ss,SSS}
        // %l -- 输出日志事件的发生位置，包括类目名、发生的线程、以及在代码中的行数
        static std::map<std::string, std::function<FormatItem::ptr(const std::string &str)> > s_format_items = {
#define XX(str, C)  {#str, [](const std::string& fmt) { return FormatItem::ptr(new C(fmt));} }

                XX(m, MessageFormatItem),
                XX(p, LevelFormatItem),
                XX(r, ElapseFormatItem),
                XX(c, LogNameFormatItem),
                XX(t, ThreadIdFormatItem),
                XX(n, NewLineFormatItem),
                XX(d, DateTimeFormatItem),
                XX(f, FilenameFormatItem),
                XX(l, LineFormatItem),
                XX(T, TabFormatItem),               //T:Tab
                XX(F, FiberIdFormatItem),           //F:协程id
                XX(N, ThreadNameFormatItem),        //N:线程名称

#undef XX
        };

        for (auto &v: patterns) {
            if (v.first == 0) {
                m_items.push_back(FormatItem::ptr(new StringFormatItem(v.second)));
            } else if (v.second == "d") {
                m_items.push_back(FormatItem::ptr(new DateTimeFormatItem(dateformat)));
            } else {
                auto it = s_format_items.find(v.second);
                if (it == s_format_items.end()) {
                    std::cout << "[ERROR] LogFormatter::init() " << "pattern: [" << m_pattern << "] " <<
                              "unknown format item: " << v.second << std::endl;
                    error = true;
                    break;
                } else {
                    m_items.push_back(it->second(v.second));
                }
            }
        }

        if (error) {
            m_error = true;
            return;
        }

    }

    LoggerManager::LoggerManager() {
        m_root.reset(new Logger);
        m_root->addAppender(LogAppender::ptr(new StdoutLogAppender));

        m_loggers[m_root->m_name] = m_root;

        init();
    }

    void LoggerManager::init() {
    }

    Logger::ptr LoggerManager::getLogger(const std::string &name) {
        //MutexType::Lock lock(m_mutex);
        auto it = m_loggers.find(name);
        if (it != m_loggers.end()) {
            return it->second;
        }

        Logger::ptr logger(new Logger(name));
        logger->m_root = m_root;
        m_loggers[name] = logger;
        return logger;
    }

    std::string LoggerManager::toYamlString() {
        //MutexType::Lock lock(m_mutex);
        YAML::Node node;
        for(auto& i : m_loggers) {
            node.push_back(YAML::Load(i.second->toYamlString()));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }


    struct LogAppenderDefine {
        int type = 0; //1 file   2 stdout
        LogLevel::Level level = LogLevel::UNKNOW;
        std::string formatter;
        std::string file;

        bool operator==(const LogAppenderDefine &oth) const {
            return type == oth.type
                   && level == oth.level
                   && formatter == oth.formatter
                   && file == oth.file;
        }
    };

    struct LogDefine {
        std::string name;
        LogLevel::Level level = LogLevel::UNKNOW;
        std::string formatter;
        std::vector<LogAppenderDefine> appenders;

        bool operator==(const LogDefine &oth) const {
            return name == oth.name
                   && level == oth.level
                   && formatter == oth.formatter
                   && appenders == oth.appenders;
        }

        bool operator<(const LogDefine &oth) const {
            return name < oth.name;
        }
    };


    template<>
    class LexicalCast<LogDefine, std::string> {
    public:
        std::string operator()(const LogDefine &p) {
            YAML::Node node;
            node["name"] = p.name;
            if (p.level != LogLevel::UNKNOW) {
                node["level"] = LogLevel::ToString(p.level);
            }
            if (!p.formatter.empty()) {
                node["formatter"] = p.formatter;
            }
            for (auto &i: p.appenders) {
                YAML::Node node1;
                if (i.type == 1) {
                    node1["type"] = "FileLogAppender";
                    node1["file"] = i.file;
                } else if (i.type == 2) {
                    node1["type"] = "StdoutLogAppender";
                }

                if (i.level != LogLevel::UNKNOW) {
                    node1["level"] = LogLevel::ToString(i.level);
                }
                if (!i.formatter.empty()) {
                    node["formatter"] = i.formatter;
                }
                node["appenders"].push_back(node1);
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };


    template<>
    class LexicalCast<std::string, LogDefine> {
    public:
        LogDefine operator()(const std::string &s) {
            YAML::Node node = YAML::Load(s);
            LogDefine p;
            if (!node["name"].IsDefined()) {
                std::cout << "log config error: name is null, " << node
                          << std::endl;
                throw std::logic_error("log config name is null");
            }
            p.name = node["name"].as<std::string>();
            p.level = LogLevel::FromString(node["level"].IsDefined() ? node["level"].as<std::string>() : "");
            if (node["formatter"].IsDefined()) {
                p.formatter = node["formatter"].as<std::string>();
            }
            if (node["appenders"].IsDefined()) {
                for (size_t i = 0; i < node["appenders"].size(); i++) {
                    auto a = node["appenders"][i];
                    if(!a["type"].IsDefined()) {
                        std::cout << "log config error: appender type is null, " << a
                                  << std::endl;
                        continue;
                    }
                    std::string type = a["type"].as<std::string>();
                    LogAppenderDefine ld;
                    if (type == "FileLogAppender") {
                        ld.type = 1;
                        if (!a["file"].IsDefined()) {
                            std::cout << "log config error: fileAppender file is null, " << a
                                      << std::endl;
                            continue;
                        }
                        ld.file = a["file"].as<std::string>();
                        if (a["formatter"].IsDefined()) {
                            ld.formatter = a["formatter"].as<std::string>();
                        }
                    } else if (type == "StdoutLogAppender") {
                        ld.type = 2;
                        if (a["formatter"].IsDefined()) {
                            ld.formatter = a["formatter"].as<std::string>();
                        }
                    } else {
                        std::cout << "log config error: appender type is invalid " << a << std::endl;
                        continue;
                    }
                    p.appenders.push_back(ld);
                }
            }
            return p;
        }
    };




    sylar::ConfigVar<std::set<LogDefine> >::ptr g_log_defines =
            sylar::Config::Lookup("logs", std::set<LogDefine>(), "logs config");

    struct LogIniter {
        LogIniter() {
            g_log_defines->addListener(0xF1E231,[](const std::set<LogDefine> &old_value,
                                             const std::set<LogDefine> &new_value) {
                SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "on_change_conf_change";
                for (auto &i: new_value) {
                    auto it = old_value.find(i);
                    sylar::Logger::ptr logger;
                    if (it == old_value.end()) {
                        //新增logger
                        logger = SYLAR_LOG_NAME(i.name);
                    } else {
                        if (!(i == *it)) {
                            //修改logger
                            logger = SYLAR_LOG_NAME(i.name);
                        } else {
                            continue;
                        }
                    }
                    logger->setLevel(i.level);
                    if (!i.formatter.empty()) {
                        logger->setFormatter(i.formatter);
                    }

                    logger->clearAppender();
                    for (auto &a: i.appenders) {
                        sylar::LogAppender::ptr ap;
                        if (a.type == 1) {
                            ap.reset(new FileLogAppender(a.file));
                        } else if (a.type == 2) {
                            ap.reset(new StdoutLogAppender);
                        } else {
                            continue;
                        }
                        ap->setLevel(a.level);
                        if(!a.formatter.empty()) {
                            LogFormatter::ptr fmt(new LogFormatter(a.formatter));
                            if(!fmt->isError()) {
                                ap->setFormatter(fmt);
                            } else {
                                std::cout << "log.name=" << i.name << " appender type=" << a.type
                                          << " formatter=" << a.formatter << " is invalid" << std::endl;
                            }
                        }
                        logger->addAppender(ap);
                    }
                }
                for (auto &i: old_value) {
                    auto it = new_value.find(i);
                    if (it == new_value.end()) {
                        //删除logger
                        auto logger = SYLAR_LOG_NAME(i.name);
                        logger->setLevel((LogLevel::Level)0);
                        //logger->setLevel(LogLevel::WARN);
                        logger->clearAppender();
                    }
                }
            });
        }
    };

    static LogIniter __log_init;

}