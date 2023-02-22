//
// Created by zgys on 23-2-20.
//
#include "log.h"
#include "scheduler.h"
#include "macro.h"

namespace sylar {

    static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

    /// 当前线程的调度器，同一个调度器下的所有线程共享同一个实例
    static thread_local Scheduler* t_scheduler = nullptr;
    /// 当前线程的调度协程，每个线程都独有一份
    static thread_local Fiber* t_scheduler_fiber = nullptr;

    Scheduler::Scheduler(size_t threads, bool use_caller, const std::string &name)
            : m_name(name),
              m_useCaller(use_caller) {
        SYLAR_ASSERT(threads > 0);

        // 是否把创建协程调度器的线程放到协程调度器管理的线程池中
        // 如果不放入，那这个线程专职协程调度
        // 如果放入， 那就要把协程调度器封装到这个线程的一个协程中，称之为主协程或协程调度器协程
        // 放入
        if (use_caller) {
            --threads;                       // 因为要放入，所以放入的线程占线程池的1个，应当创建的线程数减一
            sylar::Fiber::GetThis();         // 返回当前协程，如果没有则创建当前线程的主协程
            SYLAR_ASSERT(GetThis() == nullptr);
            t_scheduler = this;              // 将当前调度器实例保存到当前线程的调度器t_scheduler
            ///把协程调度器封装到一个协程中，绑定当前调度器实例的的run方法作为协程的回调函数（执行函数）
            /**
             * caller线程的主协程不会被线程的调度协程run进行调度，而且，线程的调度协程停止时，应该返回caller线程的主协程
             * 在user caller情况下，把caller线程的主协程暂时保存起来，等调度协程结束时，再resume caller协程
             */
            /// tips,这里在将调度器所在协程(主协程)绑定run方法构造时，不受调度器调度，否则fiber的析构会出问题
            /// 由于use caller，此线程要被放入线程池，所以要创建一个运行run方法的协程调度器(主协程)，用m_rootFiber指向它，主协程是没有栈的，
            /// 用的就是线程的栈，stacksize = 0， 不在调度器中跑，它自己就在调度别的协程
            m_rootFiber.reset(new Fiber(std::bind(&Scheduler::run, this), 0, false));

            sylar::Thread::SetName(m_name);  // 将线程名设置为协程名
            t_scheduler_fiber = m_rootFiber.get(); // 将调度器协程保存起来
            m_rootThread = sylar::GetThreadId();   // 调度器所在线程id
            m_threadIds.push_back(m_rootThread);   // 将调度器所在线程id加入线程id池
        } else {  // 不放入
            m_rootThread = -1;
        }
        m_threadCount = threads;
    }

    Scheduler::~Scheduler() {
        SYLAR_LOG_DEBUG(g_logger) << "Scheduler::~Scheduler()";
        SYLAR_ASSERT(m_stopping);
        if(GetThis() == this) {      // 如果当前实例就是调度器协程
            t_scheduler = nullptr;   // 当前线程的调度器置空
        }
    }

    Scheduler *Scheduler::GetThis() {
        return t_scheduler;
    }

    Fiber *Scheduler::GetMainFiber() {
        return t_scheduler_fiber;
    }

    void Scheduler::start() {
        SYLAR_LOG_DEBUG(g_logger) << "start";
        MutexType::Lock lock(m_mutex);
        if(m_stopping) {
            SYLAR_LOG_ERROR(g_logger) << "Scheduler is stopped";
            return;
        }

        SYLAR_ASSERT(m_threads.empty());         // 线程池中如果非空，失败

        m_threads.resize(m_threadCount); // 线程池大小设置为要求数量
        for(size_t i = 0; i < m_threadCount; i++) {
            //创建线程加入线程池，线程池中的线程执行的回调函数是调度器的run来调度线程中的协程
            m_threads[i].reset(new Thread(std::bind(&Scheduler::run, this),
                                          m_name + "_" + std::to_string(i)));
            //创建的线程的线程id加入线程id池
            m_threadIds.push_back(m_threads[i]->getId());
        }
        lock.unlock();
    }

    void Scheduler::stop() {
        SYLAR_LOG_INFO(g_logger) << this << " stop";
        if(stopping()) {
            return;
        }
        m_stopping = true;

        /// 如果use caller，那只能由caller线程发起stop
        if(m_useCaller) {          // GetThis()返回调度器所在线程，使用了use_caller,一定要在创建schedule的线程中执行stop
            SYLAR_ASSERT(GetThis() == this);
        } else {                   // 没有使用，说明可以在任意非它的线程中执行stop
            SYLAR_ASSERT(GetThis() != this);
        }

        // 通知所有线程的协程调度器，有任务的，将任务队列中的任务执行完
        for(size_t i = 0; i < m_threadCount; i++) {
            tickle();
        }

        // 执行完caller线程中的任务
        if(m_rootFiber){
            tickle();
        }

        /// 在use caller情况下，调度器协程结束时，应该返回caller协程
        if (m_rootFiber) {
            m_rootFiber->resume();
            SYLAR_LOG_DEBUG(g_logger) << "m_rootFiber end";
        }

        // 创建一个空的线程池，以swap的方式使Thread的智能指针引用计数不变
        std::vector<Thread::ptr> thrs;
        {
            MutexType::Lock lock(m_mutex);
            thrs.swap(m_threads);
        }
        // 线程池中的线程以join的方式结束
        for (auto &i : thrs) {
            i->join();
        }
    }

    void Scheduler::tickle() {
        SYLAR_LOG_DEBUG(g_logger) << "ticlke";
    }

    bool Scheduler::stopping() {
        MutexType::Lock lock(m_mutex);
        return m_stopping && m_tasks.empty() && m_activeThreadCount == 0;
    }

    void Scheduler::idle() {
        SYLAR_LOG_DEBUG(g_logger) << "idle";
        while (!stopping()) {
            sylar::Fiber::GetThis()->yield();
        }
    }

    void Scheduler::setThis() {
        t_scheduler = this;
    }

    void Scheduler::run() {
        SYLAR_LOG_DEBUG(g_logger) << "run";
        setThis();        //把当前线程的schedule设置为自己

        if(sylar::GetThreadId() != m_rootThread) {
            t_scheduler_fiber = Fiber::GetThis().get();  //将线程的调度协程（主协程）保存到 t_scheduler_fiber
        }
        // 创建一个执行空闲任务的协程
        Fiber::ptr idle_fiber(new Fiber(std::bind(&Scheduler::idle, this)));
        // 创建一个回调任务的协程
        Fiber::ptr cb_fiber;

        ScheduleTask task;            // 创建一个任务
        while(true) {
            task.reset();             // 清空此任务
            bool tickle_me = false;   // 是否tickle其他线程进行任务调度
            {
                MutexType::Lock lock(m_mutex);
                auto it = m_tasks.begin();
                // 遍历任务队列中的所有任务
                while(it != m_tasks.end()) {
                    if(it->thread != -1       // 任务的thread不等于-1，说明指定了执行线程的id
                    && it->thread != sylar::GetThreadId()){
                        // 指定了调度线程，但不是在当前线程上调度，标记一下需要通知其他线程进行调度，然后跳过这个任务，继续下一个
                        ++it;
                        tickle_me = true; //标记一下需要通知其他线程进行调度
                        continue;
                    }

                    // 找到一个未指定线程，或是指定了当前线程的任务
                    SYLAR_ASSERT(it->fiber || it->cb);

                    // 如果选到的协程正在执行，跳过
                    if(it->fiber && it->fiber->getState() == Fiber::RUNNING) {
                        ++it;
                        continue;
                    }

                    // 当前调度线程找到一个任务，准备开始调度，将其从任务队列中剔除，活动线程数加1
                    task = *it;                   // 取出任务
                    m_tasks.erase(it);     // 从任务队列中删除任务
                    it++;                         // 迭代器继续遍历
                    ++m_activeThreadCount;        // 活跃线程数增加
                    break;
                }
                // 当前线程拿完一个任务后，发现任务队列还有剩余，那么tickle一下其他线程
                // it != m_tasks.end() 有剩余为true，按位或，tickle_me = true
                tickle_me |= (it != m_tasks.end());
            }

            if(tickle_me) {
                tickle();
            }

            if(task.fiber) {
                // resume协程，resume返回时，协程要么执行完了，要么半路yield了，总之这个任务就算完成了，活跃线程数减一
                task.fiber->resume();
                --m_activeThreadCount;
                task.reset();
            } else if (task.cb) {
                if(cb_fiber) { /// 执行cb的协程可以用，调用的是fiber中的reset
                    cb_fiber->reset(task.cb); // cb_fiber协程复用绑定任务task.cb
                } else {       /// 不可用，cb_fiber协程指向绑定任务task.cb的新协程 调用的是 shared_ptr的reset
                    // reset()包含两个操作。当智能指针中有值的时候，调用reset()会使引用计数减1.
                    // 当调用reset（new xxx())重新赋值时，智能指针首先是生成新对象，
                    // 然后将旧对象的引用计数减1（当然，如果发现引用计数为0时，则析构旧对象），
                    // 然后将新对象的指针交给智能指针保管。
                    cb_fiber.reset(new Fiber(task.cb));
                }
                task.reset();            // 重置(清空)此任务
                cb_fiber->resume();      // 唤醒，执行cb_fiber
                m_activeThreadCount--;
                cb_fiber.reset();        // 引用计数减一
            } else {
                // 进到这个分支情况一定是任务队列空了，调度idle协程即可
                if(idle_fiber->getState() == Fiber::TERM) {
                    // 如果调度器没有调度任务，那么idle协程会不停地resume/yield，不会结束，如果idle协程结束了，那一定是调度器停止
                    SYLAR_LOG_DEBUG(g_logger) << "idle fiber term";
                    break;
                }

                ++m_idleThreadCount;
                idle_fiber->resume();
                --m_idleThreadCount;
            }
        }
        SYLAR_LOG_DEBUG(g_logger) << "Scheduler::run() exit";
    }
}