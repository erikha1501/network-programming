#pragma once

#include <exception>

namespace core::exception
{

class SocketException : public std::exception
{
    const char* what() const noexcept final override
    {
        return "Socket exception";
    }
};

class ThreadAbortedException : public std::exception
{
    const char* what() const noexcept final override
    {
        return "Thread aborted";
    }
};

} // namespace core::exception
