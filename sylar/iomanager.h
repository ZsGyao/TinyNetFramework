/**
  ********************************************************
  * @file        : iomanager.h
  * @author      : zgys
  * @brief       : None
  * @attention   : 底层基于epoll实现
  * @date        : 23-2-22
  ********************************************************
  */
#ifndef __SYLAR_IOMANAGER_H__
#define __SYLAR_IOMANAGER_H__

#include "scheduler.h"

namespace sylar {
    class IOManager : public Scheduler {
    public:
        typedef std::shared_ptr<IOManager> ptr;
        typedef RWMutex RWMutexType;

        /**
         * @brief   IO事件
         */
        enum Event {
            NONE  = 0x0,   // 无事件
            READ  = 0x1,   // 读事件(EPOLLIN)
            WRITE = 0x2    // 写事件(EPOLLOUT)
        };

    private:
        /**
         * @brief socket fd 上下文类
         * @details 每个socket fd都对应一个FdContext，包括fd的值，fd上的事件，以及fd的读写事件上下文中
         */
        struct FdContext {
            typedef Mutex MutexType;
            /**
             * @brief 事件上下文类
             * @details fd的每个事件都有一个事件上下文，保存这个事件的回调函数以及执行回调函数的调度器
             *          sylar对fd事件做了简化，只预留了读事件和写事件，所有的事件都被归类到这两类事件中
             */
            struct  EventContext {
                Scheduler*            scheduler = nullptr;  // 事件执行的scheduler
                Fiber::ptr            fiber;                // 事件协程
                std::function<void()> cb;                   // 事件的回调函数
            };

            /**
             * @brief 获取事件上下文类
             * @param[in] event 事件类型
             * @return 返回对应事件的上下文
             */
            EventContext& getEventContext(Event event);
            EventContext& resetEventContext(EventContext& event_ctx);

            EventContext read;                 // 读事件上下文
            EventContext write;                // 写事件上下文
            int          fd = 0;               // 事件关联的句柄
            Event        events = NONE;        // 该fd添加了哪些事件的回调函数
            MutexType    mutex;                // 互斥锁
        };

    public:
        /**
         * @brief 构造函数
         * @param threads 线程数量
         * @param use_caller 是否将调用线程包含进去
         * @param name 调度器的名称
         */
        IOManager(size_t threads = 1, bool use_caller = true, const std::string &name = "IOManager");

        /**
         * @brief 析构函数
         */
        ~IOManager();

        /**
         * @brief 添加事件
         * @param fd 句柄
         * @param event 事件
         * @param cb 事件回调函数，如果为空，则默认把当前协程作为回调执行体
         * @return 1 success  0 retry  -1 error
         */
        int addEvent(int fd, Event event, std::function<void()> cb = nullptr);

        /**
         * @brief 删除事件
         * @param fd 句柄
         * @param event 事件
         * @return 是否删除成功
         */
        bool delEvent(int fd, Event event);

        /**
         * @brief 取消事件 （强制触发，并取消事件）
         * @param fd
         * @param event
         * @return 是否删除成功
         */
        bool cancelEvent(int fd, Event event);

        /**
         * @brief 强制触发，并取消一个句柄下所有事件
         * @param fd
         * @return 是否删除成功
         */
        bool cancelAll(int fd);

        /**
         * @brief 获取当前的IOManager
         */
        static IOManager* GetThis();

    protected:
       /**
        * @brief 通知调度器有任务要调度
        * @details 写pipe让idle协程从epoll_wait退出，待idle协程yield之后Scheduler::run就可以调度其他任务
        */
        void tickle() override;

        /**
         * @brief 判断是否可以停止
         * @details 判断条件是Scheduler::stopping()外加IOManager的m_pendingEventCount为0，
         *          表示没有IO事件可调度了
         * @return 是否可以停止
         */
        bool stopping() override;

        /**
         * @brief idle协程
         * @details 对于IO协程调度来说，应阻塞在等待IO事件上，
         *          idle退出的时机是epoll_wait返回，对应的操作是tickle或注册的IO事件发生
         */
        void idle() override;

        /**
         * @brief 重置socket句柄上下文的容器大小
         * @param size 容量大小
         */
        void contextResize(size_t size);

    private:
        int                     m_epfd = 0;                      // epoll的句柄
        int                     m_tickleFds[2];                  // pipe的句柄，fd[0]读端，fd[1]写端
        std::atomic<size_t>     m_pendingEventCount = {0};     // 等待执行的IO事件数
        RWMutexType             m_mutex;                        // 读写锁
        std::vector<FdContext*> m_fdContexts;                   // socket事件上下文的容器
    };


}

#endif //SYLAR_IOMANAGER_H
