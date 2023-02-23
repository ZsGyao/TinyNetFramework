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

    Timer::ptr TimerManager::addTimer(uint64_t ms, std::function<void()> cb, bool recurring) {
        Timer::ptr timer(new Timer(ms, cb, recurring, this));
        RWMutexType::WriteLock lock(m_mutex);
        auto it = m_timers.insert(timer).first;
        bool at_front = (it == m_timers.begin());
        lock.unlock();

        if(at_front) {
            onTimerInsertedAtFront();
        }
        return timer;
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
        if(m_timers.empty()) {
            return ~0ull;     //如果定时器的队列是空的，那就返回一个最大的数， 0取反 ull是unsigned long long
        }

        const Timer::ptr& next = *m_timers.begin();
        uint64_t now_ms = sylar::GetCurrentMS();
        if(now_ms > next->m_next) {
            return 0;
        } else {
            return next->m_next;
        }
    }

    void TimerManager::listExpiredCb(std::vector<Timer::ptr>& cbs) {
        uint64_t now_ms = sylar::GetCurrentMS();
        std::vector<Timer::ptr> expired;
        {
            RWMutexType::ReadLock lock(m_mutex);
            if(m_timers.empty()) {
                return;
            }
        }
        RWMutexType::WriteLock lock(m_mutex);
        
    }
}