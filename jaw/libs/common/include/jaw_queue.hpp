#ifndef JAW_QUEUE_H
#define JAW_QUEUE_H

#include <limits>
#include <chrono>
#include <queue>
#include <mutex>
#include <condition_variable>

namespace Jaw {

/*************************************************************************************************/

// Queue that can be accessed concurrently by many threads.
template<typename T>
class Queue
{
public:

    // Create new queue with maximum limit. Default: unlimited.
    explicit Queue(std::size_t max_size = std::numeric_limits<std::size_t>::max())
        : max_size_(max_size)
        , queue_()
        , mutex_()
        , pushed_condition_()
        , popped_condition_()
    {}

    // Disable copy operations.
    Queue(const Queue&) = delete;
    Queue& operator=(const Queue&) = delete;

    // Push new item to queue.
    // Returns true if item was successfully pushed, false if timed-out (queue was full).
    template <typename Rep, typename Period>
    bool push(const T& item, const std::chrono::duration<Rep, Period>& timeout)
    {
        std::unique_lock<std::mutex> lock(mutex_);
        if (!popped_condition_.wait_for(lock, timeout, [this]() { return queue_.size() < max_size_; })) {
            return false;
        }
        queue_.push(item);
        pushed_condition_.notify_one();
        return true;
    }

    // Pop oldest item from queue saving copy in supplied reference.
    // Returns true if item was successfully popped, false if timed-out (queue was empty).
    template <typename Rep, typename Period>
    bool pop(T& item, const std::chrono::duration<Rep, Period>& timeout)
    {
        std::unique_lock<std::mutex> lock(mutex_);
        if (!pushed_condition_.wait_for(lock, timeout, [this]() { return !queue_.empty(); })) {
            return false;
        }
        item = queue_.front();
        queue_.pop();
        popped_condition_.notify_one();
        return true;
    }

    // Gets the number of items in the queue.
    std::size_t size()
    {
        std::unique_lock<std::mutex> lock(mutex_);
        return queue_.size();
    }

private:

    // Maximum number of items that can be stored in queue. */
    std::size_t max_size_;

    // Queue holding items.
    std::queue<T> queue_;

    // Mutex used to synchronize operations.
    std::mutex mutex_;

    // Notify that a new item was pushed.
    std::condition_variable pushed_condition_;

    // Notify that an old item was popped.
    std::condition_variable popped_condition_;
};

/*************************************************************************************************/

}

#endif // JAW_QUEUE_H
