#include "Rand.h"

#include "MemoryLoggerOn.h"
namespace tim
{
namespace core
{
    std::default_random_engine Rand::s_generator;
    std::uniform_real<float> Rand::frand_range;
}
}
