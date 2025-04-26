#ifndef THREADPOOL_H_INCLUDED
#define THREADPOOL_H_INCLUDED

#include "thread_pool/thread_pool.h"
#include "Singleton.h"
#include "NonCopyable.h"
#include <mutex>
#include <future>

#include "MemoryLoggerOn.h"
namespace tim
{
namespace core
{
    class ThreadPool : NonCopyable
    {
    public:
        ThreadPool() : _pool(std::thread::hardware_concurrency()) {}
        ThreadPool(size_t poolSize) : _pool(poolSize) {}
        ~ThreadPool() = default;

        template <class T>
        ThreadPool& schedule(const T& task)
        {
            std::lock_guard guard(_mutex);
            _pool.enqueue(task);
            return *this;
        }

        template <class TaskType>
        std::future<decltype((*((TaskType*)nullptr))())> schedule_trace(const TaskType& task) // TODO
        {
            std::lock_guard guard(_mutex);
            std::shared_ptr<std::promise<decltype((*((TaskType*)nullptr))())>> prom(new std::promise<decltype((*((TaskType*)nullptr))())>());
            _pool.enqueue([=](){prom->set_value(task());});
            return prom->get_future();

        }

        // bool empty() const;
        // size_t pending() const;
        // size_t active() const;
        // 
        void wait() { std::lock_guard guard(_mutex); _pool.wait_for_tasks(); }

    private:
        dp::thread_pool<> _pool;
        mutable std::mutex _mutex;
    };

    // inline bool ThreadPool::empty() const { std::lock_guard(_mutex); return _pool->empty(); }
    // inline size_t ThreadPool::pending() const { std::lock_guard(_mutex); return _pool->pending(); }
    // inline size_t ThreadPool::active() const { std::lock_guard(_mutex); return _pool->active(); }
}
}
#include "MemoryLoggerOff.h"

#endif // THREADPOOL_H_INCLUDED
