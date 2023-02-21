//
// Created by zgys on 23-2-20.
//  协程调度器
//

#ifndef __SYLAR_SCHEDULE_H__
#define __SYLAR_SCHEDULE_H__

#include <memory>
#include <vector>
#include "fiber.h"
#include "mutex.h"
#include "thread.h"

namespace sylar {
    class Schedule {
    public:
        typedef std::shared_ptr<Schedule> ptr;
        typedef Mutex MutexType;

        // threads 线程数量  use_caller 是否使用当前调用线程  name 协程调度器名称
        Schedule(size_t threads = 1, bool use_caller = true, const std::string &name = "");
        virtual ~Schedule();

        const std::string &getName() const { return m_name; }
        static Schedule* GetThis();        // 返回当前协程调度器
        static Fiber* GetMainFiber();      // 返回当前协程调度器的调度协程

        void start();
        void stop();

        /**
         * @brief 调度协程
         * @param[in] fc 协程或函数
         * @param[in] thread 协程执行的线程id,-1标识任意线程
         */
        template<class FiberOrCb>
        void  schedule(FiberOrCb fc, int thread = -1) {
            bool need_tickle = false;
            {
                MutexType::Lock lock(m_mutex);
                need_tickle = scheduleNoLock(fc, thread);
            }
            
            if(need_tickle) {
                tickle();
            }
        }

        /**
         * @brief 批量调度协程
         * @param[in] begin 协程数组的开始
         * @param[in] end 协程数组的结束
         */
        template<class InputIterator>
        void schedule(InputIterator begin, InputIterator end) {
            bool need_tickle = false;
            {
                MutexType::Lock lock(m_mutex);
                while (begin != end) {
                    need_tickle = scheduleNoLock(&*begin) || need_tickle;
                }
            }
            if(need_tickle) {
                tickle();
            }
        }

    protected:
        virtual void tickle();                                   // 通知协程调度器有任务了
        virtual bool stopping();                                 // 返回是否可以停止
        virtual void idle();                                     // 协程无任务可调度时执行idle协程
        void run();                                              // 协程调度函数
        void setThis();                                          // 设置当前的协程调度器
        bool hasIdleThreads() { return m_idleThreadCount > 0; }  // 是否有空闲线程

    private:
        //协程调度启动(无锁)
        template<class FiberOrCb>
        bool scheduleNoLock(FiberOrCb fc, int thread = -1) {
           bool need_tickle = m_tasks.empty();                   // 任务队列是否为空
            ScheduleTask ft(fc, thread);                         // 创建任务
           if(ft.fiber || ft.cb) {                               // 如果协程和协程的执行函数都存在
               m_tasks.push_back(ft);                            // 将此任务加入任务队列
           }
           return need_tickle;                                   // 返回true，通知schedule调度协程有任务，进行调度
        }

    private:
        // 调度任务： 协程/函数/线程组  主要由两种任务
        // 一种是已经有回调的协程fiber， 放入任务队列中，调度器调度后执行
        // 一种是回调函数cb， 放入任务队列，调度后创建一个执行它的协程执行
        struct ScheduleTask {
            Fiber::ptr fiber;                                   // 协程
            std::function<void()> cb;                           // 协程执行函数
            int thread;                                         // 指定此线程id去执行任务，未指定为 -1

            ScheduleTask(Fiber::ptr f, int thr)                 // 传入协程智能指针，指定线程号的构造函数
                    : fiber(f),
                      thread(thr) {
            }

            ScheduleTask(Fiber::ptr *f, int thr)               // 传入协程智能指针的指针，指定线程号的构造函数
                    : thread(thr) {
                fiber.swap(*f);                                //  不会使fiber的引用增多
            }

            ScheduleTask(std::function<void()> f, int thr)    // 传入协执行函数，指定线程号的构造函数
                    : cb(f),
                      thread(thr) {
            }

            ScheduleTask(std::function<void()>* f, int thr)  // 传入协执行函数的指针，指定线程号的构造函数
                    : thread(thr) {
                cb.swap(*f);
            }

            ScheduleTask()                                   // stl 使用的无参构造
                    : thread(-1) {

            }

            void reset() {
                fiber = nullptr;
                cb = nullptr;
                thread = -1;
            }
        };

    private:
        std::string              m_name;                      // 调度器名称
        MutexType                m_mutex;                     // 互斥锁
        std::vector<Thread::ptr> m_threads;                   // 线程池
        std::vector<int>         m_threadIds;                 // 线程池中线程id的集合
        std::list<ScheduleTask>  m_tasks;                     // 任务队列
        size_t                   m_threadCount = 0;           // 线程池中线程总数，不包含use_caller的主线程
        std::atomic<size_t>      m_activeThreadCount = {0};   // 活跃线程数
        std::atomic<size_t>      m_idleThreadCount = {0};     // 空闲线程数
        Fiber::ptr               m_rootFiber;                 // 调度器所在协程为主协程
        int                      m_rootThread = 0;            // use_caller为true时，调度器所在线程的id
        bool                     m_useCaller;                 // 是否使用use_caller
        bool                     m_stopping = false;          // 是否正在停止

    };
}

#endif //SYLAR_SCHEDULE_H
