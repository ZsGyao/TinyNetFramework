//
// Created by zgys on 23-2-19.
//
#include "fiber.h"
#include "config.h"
#include "macro.h"
#include "log.h"
#include "scheduler.h"
#include <atomic>

namespace sylar {

    static Logger::ptr g_logger = SYLAR_LOG_NAME("system");

    /// 全局静态变量，用于生成协程id
    static std::atomic<uint64_t> s_fiber_id {0};
    /// 全局静态变量，用于统计当前的协程数
    static std::atomic<uint64_t> s_fiber_count {0};

    /// 线程局部变量，当前线程正在运行的协程
    static thread_local Fiber* t_fiber = nullptr;
    /// 线程局部变量，当前线程的主协程，切换到这个协程，就相当于切换到了主线程中运行，智能指针形式
    static thread_local Fiber::ptr t_threadFiber = nullptr;

    // 协程栈大小，可通过配置文件获取，默认128k
    static ConfigVar<uint32_t>::ptr g_fiber_stack_size =
            Config::Lookup<uint32_t>("fiber.stack_size", 128 * 1024, "fiber stack size");

    /**
     * @brief malloc栈内存分配器，实际协程栈在堆上
     */
    class MallocStackAllocator {
    public:
        static void* Alloc(size_t size) {
            return malloc(size);
        }

        static void Dealloc(void* vp, size_t size) {
            return free(vp);
        }
    };

    using StackAllocator = MallocStackAllocator;

    Fiber::Fiber() { // 主协程
        SetThis(this);
        m_state = RUNNING;

        if(getcontext(&m_ctx)) {
            SYLAR_ASSERT2(false, "getcontext");
        }

        ++s_fiber_count;
        m_id = s_fiber_id++;
        SYLAR_LOG_DEBUG(g_logger) << "Fiber::Fiber main id = " << m_id;
    }

    Fiber::Fiber(std::function<void()> cb, size_t stacksize, bool run_in_scheduler)
    :m_id(s_fiber_id++),                        // 私有构造时s_fiber_id已经加一
     m_cb(cb),
     m_runInSchedule(run_in_scheduler){         // 子协程，需要回到函数和栈空间（栈空间实际从堆中分配）
        ++s_fiber_count;                        // 协程数增加
                                                // 在堆中分配协程栈空间
        m_stacksize = stacksize ? stacksize : g_fiber_stack_size->getValue();
        m_stack = StackAllocator::Alloc(m_stacksize);

        if(getcontext(&m_ctx)) {            // 获取上下文，放入成员m_ctx中
            SYLAR_ASSERT2(false, "getcontext");
        }
        m_ctx.uc_link          = nullptr;
        m_ctx.uc_stack.ss_sp   = m_stack;
        m_ctx.uc_stack.ss_size = m_stacksize;

        makecontext(&m_ctx, &Fiber::MainFunc, 0);
        SYLAR_LOG_DEBUG(g_logger) << "Fiber::Fiber() id = " << m_id;
    }

    Fiber::~Fiber(){
//        SYLAR_LOG_DEBUG(g_logger) << "Fiber::~Fiber start id=" << m_id
//                                  << " total=" << s_fiber_count;

        --s_fiber_count;
        if(m_stack) {                           // 有栈，说明是子协程，需要确保子协程一定是结束状态
            SYLAR_ASSERT(m_state == TERM);
            StackAllocator::Dealloc(m_stack, m_stacksize);
            SYLAR_LOG_DEBUG(g_logger) << "Dealloc stack, id = " << m_id;
        } else {                                // 无栈，说明是主协程
            SYLAR_ASSERT(!m_cb);                // 主协程不应该有回调
            SYLAR_ASSERT(m_state == RUNNING);   // 在主协程时，主协程一定在执行状态

            Fiber* cur = t_fiber;               // 当前协程就是自己
            if(cur == this) {
                SetThis(nullptr);
            }
        }

        SYLAR_LOG_DEBUG(g_logger) << "Fiber::~Fiber id=" << m_id
                                  << " total=" << s_fiber_count;
    }

    // 为了简化状态管理，强制只有TERM状态的协程才可以重置，但其实刚创建好但没执行过的协程也应该允许重置的
    void Fiber::reset(std::function<void()>cb){
        SYLAR_ASSERT(m_stack);                  // 子协程才能重置状态
        SYLAR_ASSERT(m_state == TERM);

        m_cb = cb;
        if(getcontext(&m_ctx)){
            SYLAR_ASSERT2(false,"getcontext");
        }

        m_ctx.uc_link          = nullptr;
        m_ctx.uc_stack.ss_sp   = m_stack;
        m_ctx.uc_stack.ss_size = m_stacksize;

        makecontext(&m_ctx, &MainFunc, 0);
        m_state = READY;
    }

    //让出
    void Fiber::yield() {
        /// 协程运行完之后会自动yield一次，用于回到主协程，此时状态已为结束状态
        SYLAR_ASSERT(m_state == RUNNING || m_state == TERM)
        SetThis(t_threadFiber.get());
        if (m_state != TERM) {
            m_state = READY;
        }

        // 如果协程参与调度器调度，那么应该和调度器的主协程进行swap，而不是线程主协程
        if (m_runInSchedule) {
            if (swapcontext(&m_ctx, &Scheduler::GetMainFiber()->m_ctx)) {
                SYLAR_ASSERT2(false, "swapcontext");
            }
        }else {
            if (swapcontext(&m_ctx, &t_threadFiber->m_ctx)) {
                SYLAR_ASSERT2(false, "swapcontext");
            }
        }
    }
    //唤醒
    void Fiber::resume() {
        SYLAR_ASSERT(m_state != RUNNING && m_state != TERM);
        SetThis(this);                        // 保存当前协程
        m_state = RUNNING;

        // 如果协程参与调度器调度，那么应该和调度器的主协程进行swap，而不是线程主协程
        if(m_runInSchedule) {
            if(swapcontext(&(Scheduler::GetMainFiber()->m_ctx),&m_ctx)) {
                SYLAR_ASSERT2(false, "swapcontext");
            }
        } else {
            if(swapcontext(&t_threadFiber->m_ctx, &m_ctx)) {
                SYLAR_ASSERT2(false, "swapcontext");
            }
        }
    }

    //设置当前协程
    void Fiber::SetThis(Fiber* f){
        t_fiber = f;
    }

    //返回当前协程
    Fiber::ptr Fiber::GetThis() {
        if(t_fiber) {                                 // 如果已经有协程了
            return t_fiber->shared_from_this();       // 返回协程的智能指针
        }
                                                      // 说明没有主协程
        Fiber::ptr main_fiber(new Fiber());         // 创建一个指向主协程的智能指针,此处使用的是私有的无参构造
        SYLAR_ASSERT(t_fiber == main_fiber.get());    // 如果不是主协程，不能返回，断言失败
        t_threadFiber = main_fiber;                   // 让 保存主协程的localthread 指向主协程
        return t_fiber->shared_from_this();           // 返回主协程的智能指针
    }

    //总协程数
    uint64_t Fiber::TotalFibers() {
        return s_fiber_count;
    }

    // 这里没有处理协程函数出现异常的情况，同样是为了简化状态管理，并且个人认为协程的异常不应该由框架处理，应该由开发者自行处理
    void Fiber::MainFunc() {
        Fiber::ptr cur = GetThis();      // GetThis()的shared_from_this()方法让引用计数加1
        SYLAR_ASSERT(cur);
//      try {

        cur->m_cb();
        cur->m_cb    = nullptr;         //使用functional包装的回调，如果bind一些参数，会使引用计数加一，应当指向nullptr
        cur->m_state = TERM;

//     } catch (std::exception& e) {
//            cur->m_state = EXCEPT;
//            SYLAR_LOG_ERROR(g_logger) << "Fiber Except: " << e.what()
//                                      << " fiber_id=" << cur->getId()
//                                      << std::endl
//                                      << sylar::BacktraceToString();
//        } catch (...) {
//            cur->m_state = EXCEPT;
//            SYLAR_LOG_ERROR(g_logger) << "Fiber Except: "
//                                      << " fiber_id=" << cur->getId()
//                                      << std::endl
//                                      << sylar::BacktraceToString();
//        }

        auto raw_ptr = cur.get();  // 手动让t_fiber的引用计数减1
        cur.reset();
        raw_ptr->yield();

        SYLAR_ASSERT2(false, "never reach there");
    }

    uint64_t Fiber::GetFiberId() {
        if(t_fiber) {
            return t_fiber->getId();
        }
        return 0;
    }
}