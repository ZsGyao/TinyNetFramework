/**
  ********************************************************
  * @file        : iomanager.cc
  * @author      : zgys
  * @brief       : None
  * @attention   : None
  * @date        : 23-2-22
  ********************************************************
  */
#include "iomanager.h"
#include "macro.h"
#include "log.h"
#include <sys/epoll.h>
#include <unistd.h>
#include <fcntl.h>
#include <string>
#include <cerrno>

namespace sylar {
    static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

    IOManager::IOManager(size_t threads, bool use_caller, const std::string& name)
            : Scheduler(threads, use_caller, name) {
        m_epfd = epoll_create(5000);
        SYLAR_ASSERT(m_epfd > 0);

        int rt = pipe(m_tickleFds);
        SYLAR_ASSERT(!rt);

        // 关注pipe读句柄的可读事件，用于tickle协程
        epoll_event event;
        memset(&event, 0, sizeof(epoll_event));
        // LT模式：对于读事件 EPOLLIN，只要socket上有未读完的数据，EPOLLIN 就会一直触发；对于写事件 EPOLLOUT，只要socket可写，EPOLLOUT 就会一直触发
        // ET模式：对于读事件 EPOLLIN，只有socket上的数据从无到有，EPOLLIN 才会触发；对于写事件 EPOLLOUT，只有在socket写缓冲区从不可写变为可写，EPOLLOUT 才会触发
        event.events = EPOLLIN | EPOLLET;
        event.data.fd = m_tickleFds[0]; // 读

        // 非阻塞方式，配合边缘触发
        rt = fcntl(m_tickleFds[0], F_SETFL, O_NONBLOCK);
        SYLAR_ASSERT(!rt);

        rt = epoll_ctl(m_epfd, EPOLL_CTL_ADD, m_tickleFds[0], &event);
        SYLAR_ASSERT(!rt);

        contextResize(32);

        start();    // 父类中的启动，当IOManager创建好了就默认启动
    }

    IOManager::~IOManager() {
        stop();
        close(m_epfd);
        close(m_tickleFds[0]);
        close(m_tickleFds[1]);

        for(auto fd_ctx : m_fdContexts) { // range for
                delete fd_ctx;
        }
    }

    void IOManager::contextResize(size_t size) {
        m_fdContexts.resize(size);

        for(size_t i = 0; i < m_fdContexts.size(); i++) {
            if(!m_fdContexts[i]) {
                m_fdContexts[i] = new FdContext;
                m_fdContexts[i]->fd = (int)i;
            }
        }
    }

    IOManager::FdContext::EventContext& IOManager::FdContext::getEventContext(Event event) {
        switch (event) {
            case IOManager::READ:
                return read;
            case IOManager::WRITE:
                return write;
            default:
                SYLAR_ASSERT2(false,"getEventContext");
        }
        throw std::invalid_argument("getEventContext invalid event");
    }

    IOManager::FdContext::EventContext& IOManager::FdContext::resetEventContext(EventContext& event_ctx) {
        event_ctx.scheduler = nullptr;
        event_ctx.fiber.reset();       // 智能指针放弃引用,不再指向那个对象
        event_ctx.cb        = nullptr;
    }

    int IOManager::addEvent(int fd, Event event, std::function<void()> cb) {
        // 找到fd对应的FdContext，如果不存在，那就分配一个
        FdContext *fd_ctx = nullptr;
        RWMutexType::ReadLock lock(m_mutex);
        if(m_fdContexts.size() > fd) {    // 说明fd存在
            fd_ctx = m_fdContexts[fd];
            lock.unlock();
        } else {
            lock.unlock();
            RWMutexType::WriteLock lock2(m_mutex);
            contextResize((size_t)(fd * 1.5)); // 1.5倍扩容
            fd_ctx = m_fdContexts[fd];
        }
        // 运行到此处，此fd对应的上下文由fd_ctx指向
        // 同一个fd不允许重复添加相同的事件
        FdContext::MutexType::Lock lock2(fd_ctx->mutex);   // 对fd的上下文结构修改，加FdContext中的互斥锁
        if(fd_ctx->events & event) {                         // 事件重复添加，断言
            SYLAR_LOG_ERROR(g_logger) << "addEvent assert fd=" << fd
                                      << " event=" << event
                                      << " fd_ctx.event=" << fd_ctx->events;
            SYLAR_ASSERT(!(fd_ctx->events & event));
        }

        // 不是同一个fd，得看这个事件是新增还是修改
        // 将新的事件加入epoll_wait，使用epoll_event的私有指针存储FdContext的位置
        int op = fd_ctx->events ? EPOLL_CTL_MOD : EPOLL_CTL_ADD; // 如果此fd的上下文的事件为空，说明没添加过事件，不为空，说明是修改事件
        epoll_event epevent;
        epevent.events = EPOLLET | fd_ctx->events | event;   // epoll事件，包括边沿触发，旧事件，新事件
        epevent.data.ptr = fd_ctx;         // FdContext

        int rt = epoll_ctl(m_epfd, op, fd, &epevent);
        if (rt) {
            SYLAR_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
                                      << op << ", " << fd << ", " << (EPOLL_EVENTS)epevent.events << "):"
                                      << rt << " (" << errno << ") (" << strerror(errno) << ") fd_ctx->events="
                                      << (EPOLL_EVENTS)fd_ctx->events;
            return -1;
        }

        // 待执行IO事件数加1
        ++m_pendingEventCount;

        // 找到这个fd的event事件对应的EventContext，对其中的scheduler, cb, fiber进行赋值
        fd_ctx->events                     = (Event)(fd_ctx->events | event);    // 将新事件与旧事件转化为Event保存到fd上下文
        FdContext::EventContext& event_ctx = fd_ctx->getEventContext(event);     // 获取事件上下文
        SYLAR_ASSERT(!event_ctx.scheduler && !event_ctx.fiber && !event_ctx.cb); // 有一个不为空断言

        // 赋值scheduler和回调函数，如果回调函数为空，则把当前协程当成回调执行体
        event_ctx.scheduler = Scheduler::GetThis();
        if (cb) {  // 传入的是回调函数
            event_ctx.cb.swap(cb);
        } else {   // 使用当前协程
            event_ctx.fiber = Fiber::GetThis();
            SYLAR_ASSERT2(event_ctx.fiber->getState() == Fiber::RUNNING, "state=" << event_ctx.fiber->getState());
        }
        return 0;
    }

    bool IOManager::delEvent(int fd, Event event) {
        // 找到fd对应的FdContext
        RWMutexType::ReadLock lock(m_mutex);
        if(m_fdContexts.size() < fd) { // 说明没有这个fd的上下文
            return false;
        }
        FdContext* fd_ctx = m_fdContexts[fd];
        lock.unlock();

        FdContext::MutexType::Lock lock2(fd_ctx->mutex);
        if(!(fd_ctx->events & event)) { // 如果这个fd并没有添加这个事件
            return false;
        }

        // 清除指定的事件，表示不关心这个事件了，如果清除之后结果为0，则从epoll_wait中删除该文件描述符
        //与运算+取反运算 0x00000n00 & ~0x00000100 把n设为0。 下要删除的位置为1
        Event new_events = (Event)(fd_ctx->events & ~event);
        int op           = new_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL; //如果new_events上还有位为1，则是修改，否则，已没有此fd关心的事件，删除
        epoll_event epevent;
        epevent.events = EPOLLET | new_events;
        epevent.data.ptr = fd_ctx;

        int rt = epoll_ctl(m_epfd, op, fd, &epevent);
        if(rt) {
            SYLAR_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
                                      << op << ", " << fd << ", " << (EPOLL_EVENTS)epevent.events << "):"
                                      << rt << " (" << errno << ") (" << strerror(rt) << ")";
            return false;
        }

        // 待执行事件数减1
        --m_pendingEventCount;
        // 重置该fd对应的event事件上下文
        fd_ctx->events = new_events;
        fd_ctx->resetEventContext(fd_ctx->getEventContext(event));
        return true;
    }

    bool IOManager::cancelEvent(int fd, Event event) {

    }

    bool IOManager::cancelAll(int fd) {

    }

    IOManager *IOManager::GetThis() {

    }
}