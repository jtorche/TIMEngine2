#ifndef RAND_H_INCLUDED
#define RAND_H_INCLUDED

#include "Vector.h"
#include <random>

#include "MemoryLoggerOn.h"
namespace tim
{
namespace core
{

    class Rand
    {
    public:

        Rand(uint seed) { _generator.seed(seed); }
        float next_f() { return float(frand_range(_generator)); }

        float next_f(const vec2& range)
        {
            if(range.x() >= range.y())
                return range.x();

            std::uniform_real_distribution<float> dis(range.x(), range.y());
            return float(dis(_generator));
        }

        size_t next_i() { return _generator(); }
        void setSeed(uint seed) { _generator.seed(seed); }

        /* Static */
        static float frand() { return float(frand_range(s_generator)); }

        static float frand(const vec2& range)
        {
            if(range.x() >= range.y())
                return range.x();

            std::uniform_real_distribution<float> dis(range.x(), range.y());
            return float(dis(s_generator));
        }

        static size_t rand() { return s_generator(); }

        static int rand(const ivec2& range)
        {
            std::uniform_int_distribution<> dis(range.x(), range.y());
            return dis(s_generator);
        }

        static void seed(uint seed) { s_generator.seed(seed); }

    private:
        std::default_random_engine  _generator;

        static std::default_random_engine s_generator;
        static std::uniform_real<float> frand_range;
    };

}
}
#include "MemoryLoggerOff.h"

#endif // RAND_H_INCLUDED
