#ifndef MEMORYLOGGER_H_INCLUDED
#define MEMORYLOGGER_H_INCLUDED

#include <cstdlib>
#include <mutex>
#include "type.h"

#include "Exception.h"

#include "MemoryLoggerOn.h"
namespace tim
{
namespace core
{
    class MemoryLogger
    {
    private:
        struct MemoryAlloc
        {
            void* ptr;
            size_t size, line;
            std::string file;
            bool isArray;
        };

    public:
        static MemoryLogger& instance();
        static void freeInstance();

		void* alloc(size_t, size_t, const std::string&, bool);
		void dealloc(void*, bool);
        void nextDealloc(size_t, const std::string&);

        bool exist(void*) const;

        void printLeak() const;

    protected:
        MemoryLogger();
        ~MemoryLogger();

    private:
        std::map<void*, MemoryAlloc> _allocatedMemorys;
        size_t _lastDeallocLine;
        std::string _lastDeallocFile;
        mutable std::recursive_mutex _mutex;

        static MemoryLogger* _instance;

        #include "MemoryLoggerOff.h"
        MemoryLogger(const MemoryLogger&) = delete;
        MemoryLogger& operator=(const MemoryLogger&) = delete;
        #include "MemoryLoggerOn.h"
    };
}
}
#include "MemoryLoggerOff.h"

#ifdef TIM_DEBUG
void* operator new(size_t size, size_t line, const std::string& file);
void* operator new[](size_t size, size_t line, const std::string& file);
void operator delete(void* ptr);
void operator delete[](void* ptr);
void operator delete(void* ptr, size_t line, const std::string& file);
void operator delete[](void* ptr, size_t line, const std::string& file);
#endif

#endif // MEMORYLOGGER_H_INCLUDED
