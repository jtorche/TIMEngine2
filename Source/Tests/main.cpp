
#include "TIMEngine2/core/core.h"

using namespace tim::core;
using namespace tim;


int main(int, char**)
{
    tim::core::init();

    new int;

    tim::core::quit();

    return 0;
}

