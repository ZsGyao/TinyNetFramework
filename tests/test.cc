#include <iostream>
#include "../sylar/log.h"
#include "../sylar/util.h"

int main(int argc, char **argv) {
    sylar::Logger::ptr logger(new sylar::Logger);
    logger->addAppender(sylar::LogAppender::ptr(new sylar::StdoutLogAppender));

    //std::cout<<"------------"<<std::endl;
    //测试init()
    //sylar::LogEvent::ptr event(
            //new sylar::LogEvent(logger, sylar::LogLevel::DEBUG, __FILE__, __LINE__, 0, sylar::GetThreadId(), 2, time(0), "test_thread"));
    //event->getSS() << "hello sylar log";
    //logger->log(sylar::LogLevel::DEBUG, event);

    //std::cout<<"------------"<<std::endl;
    //测试FileLogAppender、宏定义、自定义输出
    sylar::FileLogAppender::ptr file_appender(new sylar::FileLogAppender("../log.txt"));
    logger->addAppender(file_appender);
    sylar::LogFormatter::ptr fmt(new sylar::LogFormatter("%d{%Y-%m-%d %H:%M:%S}%T%p%T%m%n")) ;
    file_appender->setFormatter(fmt);
    file_appender->setLevel(sylar::LogLevel::ERROR);

    std::cout<<"------------"<<std::endl;
    SYLAR_LOG_INFO(logger) << "-----test INFO";
    SYLAR_LOG_WARN(logger) << "-----test WARN";
    SYLAR_LOG_ERROR(logger) << "-----test ERROR";
    SYLAR_LOG_DEBUG(logger) << "-----test DEBUG";
    SYLAR_LOG_FATAL(logger) << "-----test FATAL";
    std::cout<<"------------"<<std::endl;


    SYLAR_LOG_FMT_ERROR(logger, "test macroo fmt error %s", "aa");
    auto l = sylar::LoggerMgr::GetInstance()->getLogger("xx");
    SYLAR_LOG_INFO(l) << "xxx";

    return 0;
}