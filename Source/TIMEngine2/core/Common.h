#pragma once
#include <iostream>

#ifdef TIM_DEBUG
#define TIM_ASSERT(cond) \
    do { if(!(cond)) { handleAssert(__LINE__, __FILE__, ""); } }while(0) 

#define TIM_ASSERT_MSG(cond, msg) \
    do { if(!(cond)) { handleAssert(__LINE__, __FILE__, msg); } }while(0) 

#else
#define TIM_ASSERT(cond)
#define TIM_ASSERT_MSG(cond)
#endif

namespace tim
{
    void handleAssert(int _line, const char* _file, const char* _msg);

    template<typename T>
    bool isPowerOf2(T _val) { return (_val & (_val - 1)) == 0; }
}
