#ifndef THREADPOOL_H_INCLUDED
#define THREADPOOL_H_INCLUDED

#include "ThreadPoolExt.h"
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
        ThreadPool() { _pool = new ThreadPoolExt(std::thread::hardware_concurrency()); }
        ThreadPool(size_t poolSize) { _pool = new ThreadPoolExt(poolSize); }
        ~ThreadPool() { delete _pool; }

        template <class T>
        ThreadPool& schedule(const T& task)
        {
            std::lock_guard guard(_mutex);
            _pool->enqueue(task);
            return *this;
        }

        template <class TaskType>
        std::future<decltype((*((TaskType*)nullptr))())> schedule_trace(const TaskType& task)
        {
            std::lock_guard(_mutex);
            std::shared_ptr<std::promise<decltype((*((TaskType*)nullptr))())>> prom(new std::promise<decltype((*((TaskType*)nullptr))())>());
            _pool->schedule([=](){prom->set_value(task());});
            return prom->get_future();

        }

        // bool empty() const;
        // size_t pending() const;
        // size_t active() const;
        // 
        // void wait(size_t threshold=0) const;

    private:
        ThreadPoolExt* _pool;
        mutable std::mutex _mutex;
    };

    // inline bool ThreadPool::empty() const { std::lock_guard(_mutex); return _pool->empty(); }
    // inline size_t ThreadPool::pending() const { std::lock_guard(_mutex); return _pool->pending(); }
    // inline size_t ThreadPool::active() const { std::lock_guard(_mutex); return _pool->active(); }

    // inline void ThreadPool::wait(size_t threshold) const { std::lock_guard(_mutex); _pool->wait(threshold); }
}
}
#include "MemoryLoggerOff.h"

#endif // THREADPOOL_H_INCLUDED
