/**
  ********************************************************
  * @file        : timer.cc
  * @author      : zgys
  * @brief       : None
  * @attention   : None
  * @date        : 23-2-23
  ********************************************************
  */
#include "timer.h"
#include "util.h"
#include "macro.h"

namespace sylar {
    bool Timer::Comparator::operator()(const Timer::ptr& lhs, const Timer::ptr& rhs) const {
        if (!lhs && !rhs) {
            return false;
        }
        if (!lhs) {
            return true;
        }
        if (!rhs) {
            return false;
        }
        if (lhs->m_next < lhs->m_next) {
            return true;
        }
        if (lhs->m_next > lhs->m_next) {
            return false;
        }
        return lhs.get() < rhs.get();
    }

    Timer::Timer(uint64_t ms, std::function<void()> cb, bool recurring, TimerManager* manager)
            : m_recurring(recurring),
              m_ms(ms),
              m_cb(cb),
              m_manager(manager) {
        m_next = sylar::GetCurrentMS() + m_ms;
    }

    Timer::Timer(uint64_t next) : m_next(next) {}

    bool Timer::cancel() {
        TimerManager::RWMutexType::WriteLock lock(m_manager->m_mutex);
        if(m_cb) {
            auto it = m_manager->m_timers.find(shared_from_this());
            m_manager->m_timers.erase(it);
            return true;
        }
        return false;
    }

    bool Timer::refresh() {
        TimerManager::RWMutexType::WriteLock lock(m_manager->m_mutex);
        if(!m_cb) {
            return false;
        }
        auto it = m_manager->m_timers.find(shared_from_this());
        if(it == m_manager->m_timers.end()) {
            return false;
        }
        m_manager->m_timers.erase(it);
        m_next = sylar::GetCurrentMS() + m_ms;
        m_manager->m_timers.insert(shared_from_this());
        return true;
    }

    bool Timer::reset(uint64_t ms, bool from_now) {
        if(ms == m_ms && !from_now) {
            return true;
        }
        TimerManager::RWMutexType::WriteLock lock(m_manager->m_mutex);
        if(!m_cb) {
            return false;
        }
        // 在定时器管理器的定时器队列中找，如果没有这个定时器实例，旧重设失败
        auto it = m_manager->m_timers.find(shared_from_this());
        if(it == m_manager->m_timers.end()) {
            return false;
        }
        m_manager->m_timers.erase(it);
        uint64_t start = 0;
        if(from_now) {
            start = sylar::GetCurrentMS();
        } else {
            start = m_next - m_ms;
        }
        m_ms = ms;
        m_next = start + m_ms;
        m_manager->addTimer(shared_from_this(), lock);
        return true;
    }

    TimerManager::TimerManager() {
        m_previousTime = sylar::GetCurrentMS();
    }

    TimerManager::~TimerManager() {}

    Timer::ptr TimerManager::addTimer(uint64_t ms, std::function<void()> cb, bool recurring) {
        Timer::ptr timer(new Timer(ms, cb, recurring, this));
        RWMutexType::WriteLock lock(m_mutex);
        addTimer(timer, lock);
        return timer;
    }

    void TimerManager::addTimer(Timer::ptr timer, RWMutexType::WriteLock& lock) {
        auto it = m_timers.insert(timer).first;
        bool at_front = (it == m_timers.begin()) && !m_tickled;
        if(at_front) {
            m_tickled = true;
        }
        lock.unlock();
        if(at_front) {
            onTimerInsertedAtFront();
        }
    }

    static void onTimer(std::weak_ptr<void> weak_cond, std::function<void()> cb) {
        std::shared_ptr<void> tmp = weak_cond.lock();
        if(tmp) {
            cb();
        }
    }

    Timer::ptr TimerManager::addConditionTimer(uint64_t ms, std::function<void()> cb,
                                 std::weak_ptr<void> weak_cond, bool recurring) {
        return addTimer(ms, std::bind(&onTimer, weak_cond, cb), recurring);
    }

    uint64_t TimerManager::getNextTimer() {
        RWMutexType::ReadLock lock(m_mutex);
        m_tickled = false;
        if(m_timers.empty()) {
            return ~0ull;     //如果定时器的队列是空的，那就返回一个最大的数， 0取反 ull是unsigned long long
        }

        const Timer::ptr& next = *m_timers.begin();
        uint64_t now_ms = sylar::GetCurrentMS();
        if(now_ms > next->m_next) {
            return 0;
        } else {
            return next->m_next - now_ms;
        }
    }

    void TimerManager::listExpiredCb(std::vector<std::function<void()> >& cbs) {
        uint64_t now_ms = sylar::GetCurrentMS();
        std::vector<Timer::ptr> expired;
        {
            RWMutexType::ReadLock lock(m_mutex);
            if(m_timers.empty()) {
                return;
            }
        }
        RWMutexType::WriteLock lock(m_mutex);

        if(m_timers.empty()) {
            return;
        }
        bool rollover = false;
        if(SYLAR_UNLIKELY(detectClockRollover(now_ms))) {
            // 使用clock_gettime(CLOCK_MONOTONIC_RAW)，应该不可能出现时间回退的问题
            rollover = true;
        }
        if(!rollover && ((*m_timers.begin())->m_next > now_ms)) {
            return;
        }

        Timer::ptr now_timer(new Timer(now_ms));
        auto it = rollover ? m_timers.end() : m_timers.lower_bound(now_timer);
        while(it != m_timers.end() && (*it)->m_next == now_ms) {
            it++;
        }
        // 把 m_timers begin 到 it 的元素插入到 expired begin 位置
        expired.insert(expired.begin(), m_timers.begin(), it);
        //删除从 m_timers.begin() 到 it 的元素
        m_timers.erase(m_timers.begin(), it);
        //更改cbs的容量，存储过期的计时器
        cbs.reserve(expired.size());

        for(auto& timer : expired) {
            cbs.push_back(timer->m_cb);
            if(timer->m_recurring) {
                timer->m_next = now_ms + timer->m_ms;
                m_timers.insert(timer);
            } else {
                timer->m_cb = nullptr;
            }
        }
    }

    bool TimerManager::detectClockRollover(uint64_t now_ms) {
        bool rollover = false;
        if(now_ms < m_previousTime && now_ms < (m_previousTime - 60 * 60 * 1000)){
            rollover = true;
        }
        m_previousTime = now_ms;
        return rollover;
    }

    bool TimerManager::hasTimer() {
        RWMutexType::ReadLock lock(m_mutex);
        return !m_timers.empty();
    }
}