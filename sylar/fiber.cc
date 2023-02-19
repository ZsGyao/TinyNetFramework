//
// Created by zgys on 23-2-19.
//
#include "fiber.h"
#include "config.h"
#include "macro.h"
#include "log.h"
#include <atomic>

namespace sylar {

    static Logger::ptr g_logger = SYLAR_LOG_NAME("system");

    static std::atomic<uint64_t> s_fiber_id {0};
    static std::atomic<uint64_t> s_fiber_count {0};

    static thread_local Fiber* t_fiber = nullptr;
    static thread_local Fiber::ptr t_threadFiber = nullptr;

    static ConfigVar<uint32_t>::ptr g_fiber_stack_size =
            Config::Lookup<uint32_t>("fiber.stack_size", 1024 * 1024, "fiber stack size");


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

    Fiber::Fiber() { //主协程
        m_state = EXEC;
        SetThis(this);

        if(getcontext(&m_ctx)) {
            SYLAR_ASSERT2(false, "getcontext");
        }

        s_fiber_count++;
        SYLAR_LOG_DEBUG(g_logger) << "Fiber::Fiber main";
    }

    Fiber::Fiber(std::function<void()> cb, size_t stacksize)
    :m_id(++s_fiber_id),
     m_cb(cb) {  //子协程，需要回到函数和栈空间（栈空间实际从堆中分配）
        ++s_fiber_count;
        m_stacksize = stacksize ? stacksize : g_fiber_stack_size->getValue();

        m_stack = StackAllocator::Alloc(m_stacksize);
        if(getcontext(&m_ctx)) {
            SYLAR_ASSERT2(false, "getcontext");
        }
        m_ctx.uc_link = nullptr;
        m_ctx.uc_stack.ss_sp = m_stack;
        m_ctx.uc_stack.ss_size = m_stacksize;

        makecontext(&m_ctx, &Fiber::MainFunc, 0);
    }

    Fiber::~Fiber(){
        s_fiber_count--;
        if(m_stack) {
            SYLAR_ASSERT(m_state == TERM || m_state == INIT || m_state == EXCEPT);

            StackAllocator::Dealloc(m_stack, m_stacksize);
        } else { //如果是主协程
            SYLAR_ASSERT(!m_cb);                      //主协程不应该有回调
            SYLAR_ASSERT(m_state == EXEC);            //在主协程时，子协程不可能在执行状态

            Fiber* cur = t_fiber;
            if(cur == this) {
                SetThis(nullptr);
            }
        }
        SYLAR_LOG_DEBUG(g_logger) << "Fiber::~Fiber id=" << m_id
                                  << " total=" << s_fiber_count;
    }

    //重置协程的函数，并重置状态 INIT TERM
    void Fiber::reset(std::function<void()>cb){
        SYLAR_ASSERT(m_stack);
        SYLAR_ASSERT(m_state == TERM || m_state == EXCEPT || m_state == INIT);

        m_cb = cb;
        if(getcontext(&m_ctx)){
            SYLAR_ASSERT2(false,"getcontext");
        }

        m_ctx.uc_link = nullptr;
        m_ctx.uc_stack.ss_sp = m_stack;
        m_ctx.uc_stack.ss_size = m_stacksize;

        makecontext(&m_ctx, &MainFunc, 0);
        m_state = INIT;
    }

    //让出
    void Fiber::yield(){
        SetThis(t_threadFiber.get());
        if(swapcontext(&m_ctx, &t_threadFiber->m_ctx)) {
            SYLAR_ASSERT2(false, "swapcontext");
        }

    }
    //唤醒
    void Fiber::resume(){
        SetThis(this);
        SYLAR_ASSERT(m_state != EXEC);
        m_state = EXEC;
        if(swapcontext(&t_threadFiber->m_ctx, &m_ctx)) {
            SYLAR_ASSERT2(false, "swapcontext");
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

    //协程切换到后台，并且设置为Ready状态
    void Fiber::YieldToReady() {
        Fiber::ptr cur = GetThis();
        cur->m_state = READY;
        cur->yield();
    }

    //协程切换到后台，并且设置为Hold状态
    void Fiber::YieldToHold() {
        SYLAR_LOG_DEBUG(g_logger) << "Fiber Hold";
        Fiber::ptr cur = GetThis();
        cur->m_state = HOLD;
        cur->yield();
    }
    //总协程数
    uint64_t Fiber::TotalFibers() {
        return s_fiber_count;
    }

    void Fiber::MainFunc() {
        Fiber::ptr cur = GetThis();
        SYLAR_ASSERT(cur);
        try {
            cur->m_cb();
            cur->m_cb = nullptr;         //使用functional包装的回调，如果bind一些参数，会使引用计数加一，应当指向nullptr
            cur->m_state = TERM;
        } catch (std::exception& e) {
            cur->m_state = EXCEPT;
            SYLAR_LOG_ERROR(g_logger) << "Fiber Except: " << e.what()
                                      << " fiber_id=" << cur->getId()
                                      << std::endl
                                      << sylar::BacktraceToString();
        } catch (...) {
            cur->m_state = EXCEPT;
            SYLAR_LOG_ERROR(g_logger) << "Fiber Except: "
                                      << " fiber_id=" << cur->getId()
                                      << std::endl
                                      << sylar::BacktraceToString();
        }
    }

    uint64_t Fiber::GetFiberId() {
        if(t_fiber) {
            return t_fiber->getId();
        }
        return 0;
    }
}