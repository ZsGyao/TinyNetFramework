/**
  ********************************************************
  * @file        : timer.h
  * @author      : zgys
  * @brief       : 定时器
  * @attention   : None
  * @date        : 23-2-23
  ********************************************************
  */
#ifndef __SYLAR_TIMER_H__
#define __SYLAR_TIMER_H__

#include <memory>
#include <set>
#include <vector>
#include "mutex.h"

namespace sylar {

    class TimerManager;

    class Timer : public std::enable_shared_from_this<Timer> {
        friend class TimerManager;
    public:
        typedef std::shared_ptr<Timer> ptr;

    private:
        /**
         * @brief 构造，私有，由TimerManager来构造
         * @param ms 时间
         * @param cb 回调函数
         * @param recurring 是否循环定时
         * @param manager
         */
        Timer(uint64_t ms, std::function<void()> cb, bool recurring, TimerManager* manager);
        Timer(uint64_t next);
    private:
        bool                  m_recurring = false;        // 是否循环定时器
        uint64_t              m_ms = 0;                   // 执行周期
        uint64_t              m_next = 0;                // 精确的执行时间
        std::function<void()> m_cb = nullptr;            // 定时器需要执行的任务
        TimerManager*         m_manager = nullptr;       // timerManager指针

    private:
        struct Comparator {
            /**
             * @brief 比较定时器的智能指针的大小(按执行时间排序)
             * @param[in] lhs 定时器智能指针
             * @param[in] rhs 定时器智能指针
             */
            bool operator() (const Timer::ptr& lhs, const Timer::ptr& rhs) const;
        };
    };

    class TimerManager {
        friend class Timer;
    public:
        typedef RWMutex RWMutexType;

        TimerManager();
        virtual ~TimerManager();

        /**
         * @brief 添加定时器
         * @param ms 执行周期
         * @param cb 回调函数
         * @param recurring 是否循环定时器
         * @return 定时器的智能指针
         */
        Timer::ptr addTimer(uint64_t ms, std::function<void()> cb, bool recurring = false);

        /**
         * @brief 按条件添加定时器
         * @param ms 执行周期
         * @param cb 回调函数
         * @param weak_cond 执行条件
         * @param recurring 是否循环定时器
         * @return
         */
        Timer::ptr addConditionTimer(uint64_t ms, std::function<void()> cb,
                                     std::weak_ptr<void> weak_cond, bool recurring = false);

        /**
         * @brief 获取下一个定时器的执行时间
         * @return 0:定时器已过期  ~0ull：定时器队列为空  else: 还需等待的时间
         */
        uint64_t getNextTimer();

        void listExpiredCb(std::vector<Timer::ptr>& cbs);
    protected:
        /**
         * @brief 当有新的定时器插入到定时器的首部,执行该函数
         */
        virtual void onTimerInsertedAtFront() = 0;

    private:
        RWMutexType m_mutex;
        std::set<Timer::ptr, Timer::Comparator> m_timers;
    };
}

#endif //SYLAR_TIMER_H
