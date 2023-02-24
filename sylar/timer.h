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

        /**
         * @brief 取消定时器
         */
        bool cancel();

        /**
         * @brief 刷新设置定时器的执行时间
         */
        bool refresh();

        /**
         * @brief 重置定时器时间
         * @param[in] ms 定时器执行间隔时间(毫秒)
         * @param[in] from_now 是否从当前时间开始计算
         */
        bool reset(uint64_t ms, bool from_now);

    private:
        /**
         * @brief 构造，私有，由TimerManager来构造
         * @param ms 时间
         * @param cb 回调函数
         * @param recurring 是否循环定时
         * @param manager
         */
        Timer(uint64_t ms, std::function<void()> cb, bool recurring, TimerManager* manager);
        explicit Timer(uint64_t next);

    private:
        bool                  m_recurring = false;        // 是否循环定时器
        uint64_t              m_ms = 0;                   // 执行周期
        uint64_t              m_next = 0;                // 精确的执行时间
        std::function<void()> m_cb = nullptr;            // 定时器需要执行的任务
        TimerManager*         m_manager = nullptr;       // timerManager指针

    private:
        /**
         * @brief 定时器比较仿函数
         */
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

        /**
         * @brief 获取需要执行的定时器的回调函数列表
         * @param[out] cbs 回调函数数组
         */
        void listExpiredCb(std::vector<std::function<void()> >& cbs);

        /**
         * @brief 是否有定时器
         */
        bool hasTimer();
    protected:
        /**
         * @brief 当有新的定时器插入到定时器的首部,执行该函数
         */
        virtual void onTimerInsertedAtFront() = 0;

        /**
         * @brief 将定时器添加到管理器中
         */
        void addTimer(Timer::ptr timer, RWMutexType::WriteLock& lock);
    private:
        /**
         * @brief 检测服务器时间是否被调后了
         */
        bool detectClockRollover(uint64_t now_ms);
    private:
        RWMutexType                             m_mutex;
        std::set<Timer::ptr, Timer::Comparator> m_timers;              /// 定时器集合
        bool                                    m_tickled = false;     /// 是否触发onTimerInsertedAtFront
        uint64_t                                m_previousTime = 0;    /// 上次执行时间
    };
}

#endif //SYLAR_TIMER_H
