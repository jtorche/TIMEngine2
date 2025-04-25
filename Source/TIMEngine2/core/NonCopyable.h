#pragma once

namespace tim
{
    class NonCopyable
    {
    protected:

        NonCopyable() = default;
        ~NonCopyable() = default;

        NonCopyable(NonCopyable&&) = default;
        NonCopyable& operator=(NonCopyable&&) = default;

    private:
        NonCopyable(const NonCopyable&) = delete;
        NonCopyable& operator=(const NonCopyable&) = delete;
    };
}