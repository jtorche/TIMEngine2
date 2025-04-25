#ifndef SPINLOCK_H_INCLUDED
#define SPINLOCK_H_INCLUDED

#include <atomic>

namespace tim
{
namespace core
{
    class SpinLock
    {
    public:
        SpinLock() : _state(Unlocked) {}

        void lock()
        {
            while (_state.exchange(Locked, std::memory_order_acquire) == Locked);
        }

        void unlock()
        {
            _state.store(Unlocked, std::memory_order_release);
        }

    private:
        typedef enum {Locked, Unlocked} LockState;
        std::atomic<LockState> _state;
    };
}
}

#endif // SPINLOCK_H_INCLUDED
