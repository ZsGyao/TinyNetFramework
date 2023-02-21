//
// Created by zgys on 23-2-19.
//

#ifndef __SYLAR_FIBER_H__
#define __SYLAR_FIBER_H__

#include <memory>
#include <functional>
#include <ucontext.h>
#include "thread.h"

namespace sylar {

class Fiber : public std::enable_shared_from_this<Fiber> {
public:
    typedef std::shared_ptr<Fiber> ptr;

    /**
     * @brief 协程状态
     * @details 在sylar基础上进行了状态简化，只定义三态转换关系，也就是协程要么正在运行(RUNNING)，
     * 要么准备运行(READY)，要么运行结束(TERM)。不区分协程的初始状态，初始即READY。不区分协程是异常结束还是正常结束，
     * 只要结束就是TERM状态。也不区别HOLD状态，协程只要未结束也非运行态，那就是READY状态。
     */
    enum State {
        READY,      /// 就绪态，刚创建或者yield之后的状态
        RUNNING,    /// 运行态，resume之后的状态
        TERM        /// 结束态，协程的回调函数执行完之后为TERM状态
    };

private:
    Fiber();

public:
    /**
     * @brief 构造函数，用于创建用户协程
     * @param[in] cb 协程入口函数
     * @param[in] stacksize 栈大小
     * @param[in] run_in_scheduler 本协程是否参与调度器调度，默认为true
     */
    Fiber(std::function<void()> cb, size_t stacksize = 0, bool run_in_scheduler = true);
    ~Fiber();

    //重置协程的函数，重置协程状态和入口函数，复用栈空间，不重新创建栈
    void reset(std::function<void()>cb);
    //让出
    void yield();
    //唤醒
    void resume();

    State getState() { return m_state; }
    uint64_t getId() const { return m_id; }
public:
    //设置当前正在运行的协程，即设置线程局部变量t_fiber的值
    static void SetThis(Fiber* f);

    /**
      * @brief 返回当前线程正在执行的协程
      * @details 如果当前线程还未创建协程，则创建线程的第一个协程，
      * 且该协程为当前线程的主协程，其他协程都通过这个协程来调度，也就是说，其他协程
      * 结束时,都要切回到主协程，由主协程重新选择新的协程进行resume
      * @attention 线程如果要创建协程，那么应该首先执行一下Fiber::GetThis()操作，以初始化主函数协程
      */
    static Fiber::ptr GetThis();

    //获取总协程数
    static uint64_t TotalFibers();
    //协程入口函数 执行完成返回到线程调度协程
    static void MainFunc();
    //获取当前协程id
    static uint64_t GetFiberId();
private:
    uint64_t              m_id = 0;                   // 协程id
    uint32_t              m_stacksize = 0;            // 协程栈大小
    State                 m_state = READY;            // 协程状态
    ucontext_t            m_ctx;                      // 协程上下文
    void*                 m_stack = nullptr;          // 协程栈地址
    std::function<void()> m_cb;                       // 协程回到函数入口
    bool                  m_runInSchedule;            // 是否由协程d
};

}

#endif //SYLAR_FIBER_H
