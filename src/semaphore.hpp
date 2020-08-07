#ifndef _SEMAPHORE_HPP_
#define _SEMAPHORE_HPP_

/**
 * C++11 required for compilation
 * This is a portable header only implementation of a semaphore with std::mutex
 * and std::condition variable
 */

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <limits>
#include <mutex>

namespace thread_utils {
template <uint32_t LIMIT>
class Semaphore {
protected:
    mutable std::mutex mMutex;
    std::condition_variable mConditionVariable;
    std::atomic<uint32_t> mCounter;
    std::atomic<uint32_t> mLimit;

public:
    Semaphore()
        : mMutex()
        , mConditionVariable()
        , mCounter(0)
        , mLimit(LIMIT)
    {
    }
    ~Semaphore()
    {
        {
            std::lock_guard<std::mutex> locker(mMutex);
            mCounter.store(0);
        }
        mConditionVariable.notify_all();
    }
    /**
   * Increment the semaphore counter by one if it is below the limit
   * @return If the semaphore counter would exceed the limit then false is
   * returned, otherwise true
   */
    bool post()
    {
        if (mCounter.load() < mLimit) // mCounter is atomic to avoid switching to
        // kernel space if the LIMIT has reached
        {
            { // locking only while incrementing mCounter to avoid waking the waiting
                // thread only to block again
                std::lock_guard<std::mutex> locker(mMutex);
                ++mCounter;
            }
            mConditionVariable.notify_all();
            return true;
        }
        else {
            return false;
        }
    }
    /**
   * Alias for post()
   */
    inline bool signal() { return post(); }
    /**
   * Alias for post()
   */
    inline bool notify() { return post(); }
    /**
   * Block the current thread until the semaphore counter rises above 0
   */
    void wait()
    {
        std::unique_lock<std::mutex> locker(mMutex);
        if (mCounter.load() > 0) {
            --mCounter;
        }
        else {
            mConditionVariable.wait(locker, [&] { return (mCounter.load() > 0); });
            if (mCounter.load() > 0) {
                --mCounter;
            }
        }
    }
    /**
   * Block the current thread until the semaphore counter rises above 0 or after
   * the specified timeout duration
   * @param timeout_ms - timeout in milliseconds
   * @return False is returned if the given time has run out.
   */
    bool wait_for(int64_t timeout_ms)
    {
        std::unique_lock<std::mutex> locker(mMutex);
        if (mCounter.load() > 0) {
            --mCounter;
            return true;
        }
        else if (timeout_ms <= 0) {
            return false;
        }
        else {
            std::chrono::milliseconds dur{ timeout_ms };
            bool waken = mConditionVariable.wait_for(
                locker, dur, [&] { return (mCounter.load() > 0); });
            if (waken && (mCounter.load() > 0)) {
                --mCounter;
            }
            return waken;
        }
    }
};

class BinarySemaphore : public Semaphore<1> {
};
class DynamicSemaphore
    : public Semaphore<std::numeric_limits<uint32_t>::max()> {
public:
    DynamicSemaphore(uint32_t limit) { mLimit.store(limit); }
    /**
   * Set the maximum number the semaphore counter can reach
   * @param limit - uint32_t
   */
    void set_limit(uint32_t limit) { mLimit.store(limit); }
    /**
   * Returns the limit of the semaphore counter
   */
    uint32_t get_limit() { return mLimit.load(); }
};
} // namespace thread_utils

using binary_semaphore_t = thread_utils::BinarySemaphore;
using semaphore_t = thread_utils::DynamicSemaphore;

#endif // _SEMAPHORE_HPP_
