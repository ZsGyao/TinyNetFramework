//
// Created by zgys on 23-2-3.
//

#ifndef __SYLAR_UTIL_H__
#define __SYLAR_UTIL_H__

#include <pthread.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <vector>
#include <string>

namespace sylar{

    pid_t GetThreadId();
    uint32_t GetFiberId();

    void Backtrace(std::vector<std::string>& bt, int size = 64, int skip = 1);
    std::string BacktraceToString(int size = 64, int skip = 2, const std::string& prefix = "");

    //时间ms
    uint64_t GetCurrentMS();
    uint64_t GetCurrentUS();
}
}

#endif //SYLAR_UTIL_H
