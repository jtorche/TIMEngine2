
#include "MemoryLogger.h"
#include "StringUtils.h"
#include "Logger.h"
#include "Exception.h"

using namespace tim::core;

MemoryLogger* MemoryLogger::_instance = nullptr;

MemoryLogger& MemoryLogger::instance()
{
    if(!_instance)
        _instance = new MemoryLogger;
    return *_instance;
}

void MemoryLogger::freeInstance()
{
    _instance->~MemoryLogger();
    free(_instance);
    _instance=0;
}


MemoryLogger::MemoryLogger(){ }

MemoryLogger::~MemoryLogger()
{
    if(_allocatedMemorys.size() > 0) // some leaks
    {
        printLeak();
    }
}

void* MemoryLogger::alloc(size_t size, size_t line, const std::string& file, bool isArray)
{
    void* ptr = malloc(size);

    std::lock_guard guard(_mutex);
    _allocatedMemorys[ptr] = {ptr, size, line, file, isArray};

    return (void*)ptr;
}

void MemoryLogger::dealloc(void* ptr, bool isArray)
{
    std::lock_guard guard(_mutex);
    auto it = _allocatedMemorys.find(ptr);
    if(it != _allocatedMemorys.end())
    {
        if(it->second.isArray != isArray)
        {
            if(isArray)
            {
                DLOG("BadDealloc, use delete instead of delete[] at line " +
                                 StringUtils(_lastDeallocLine).str() + " in " + _lastDeallocFile + "\n");
                TIM_ASSERT(false);
            }
            else
            {
                DLOG("BadDealloc, use delete[] instead of delete at line " +
                                 StringUtils(_lastDeallocLine).str() + " in " + _lastDeallocFile + "\n");
                TIM_ASSERT(false);
            }
        }
        else
        {
            _allocatedMemorys.erase(it);
            free(ptr);
        }
    }
    else if(ptr != this)
    {
        /** Critical zone, never do another things than free(ptr) */
        free(ptr);
    }
}

void MemoryLogger::nextDealloc(size_t line, const std::string& file)
{
    std::lock_guard guard(_mutex);
    _lastDeallocLine = line;
    _lastDeallocFile = file;
}

void MemoryLogger::printLeak() const
{
    DLOG("\nLeaks detected:\n");

    std::lock_guard guard(_mutex);
    for(auto it = _allocatedMemorys.begin() ; it != _allocatedMemorys.end() ; ++it)
    {
        DLOG("Leak of size " + StringUtils(it->second.size).str() + " in " +
            it->second.file + " : " + StringUtils(it->second.line).str() + " (" +
             StringUtils(it->second.ptr).str() + ")\n");
    }
}

bool MemoryLogger::exist(void* ptr) const
{
    std::lock_guard guard(_mutex);
    if(_allocatedMemorys.find(ptr) != _allocatedMemorys.end())
        return true;
    else
        return false;
}

#ifdef TIM_DEBUG
void* operator new(size_t size, size_t line, const std::string& file)
{
    return MemoryLogger::instance().alloc(size, line, file, false);
}

void* operator new[](size_t size, size_t line, const std::string& file)
{
    return MemoryLogger::instance().alloc(size, line, file, true);
}

void operator delete(void* ptr, size_t line, const std::string& file)
{
    MemoryLogger::instance().dealloc(ptr, false);
}
void operator delete[](void* ptr, size_t line, const std::string& file)
{
    MemoryLogger::instance().dealloc(ptr, true);
}
#endif
