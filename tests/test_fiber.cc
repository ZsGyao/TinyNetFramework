//
// Created by zgys on 23-2-19.
//
#include "sylar/sylar.h"

sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

void run_in_fiber() {
    SYLAR_LOG_INFO(g_logger) << "run_in_fiber begin" << std::endl;
    sylar::Fiber::YieldToHold();
    SYLAR_LOG_INFO(g_logger) << "run_in_fiber end" << std::endl;
}

int main(int argc, char** argv) {
    sylar::Fiber::GetThis();
    SYLAR_LOG_INFO(g_logger) << "main begin" << std::endl;
    sylar::Fiber::ptr fiber(new sylar::Fiber(run_in_fiber));
    fiber->resume();
    SYLAR_LOG_INFO(g_logger) << "main after resume" << std::endl;
    fiber->resume();
    SYLAR_LOG_INFO(g_logger) << "main after end" << std::endl;

    return 0;
}