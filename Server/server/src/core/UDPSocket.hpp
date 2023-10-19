#pragma once

#include "IPEndpoint.hpp"

#include "CommonException.hpp"

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <errno.h>
#include <assert.h>
#include <stdio.h>

class UDPSocket
{
public:
    UDPSocket(const UDPSocket&) = delete;
    UDPSocket& operator=(const UDPSocket&) = delete;

    UDPSocket(UDPSocket&&) = default;
    UDPSocket& operator=(UDPSocket&&) = default;

    UDPSocket(uint16_t bindedPort = 0, bool blocking = true)
    {
        int socketType = SOCK_DGRAM | (blocking ? 0 : SOCK_NONBLOCK);
        m_socketFd = socket(AF_INET, socketType, IPPROTO_UDP);

        assert(m_socketFd >= 0);
        if (m_socketFd == -1)
        {
            throw;
        }

        sockaddr_in sockAddr{};
        sockAddr.sin_family = AF_INET;
        sockAddr.sin_port = htons(bindedPort);
        sockAddr.sin_addr.s_addr = htonl(INADDR_ANY);

        int ret = bind(m_socketFd, (sockaddr*)&sockAddr, sizeof(sockAddr));

        assert(ret >= 0);
        if (ret == -1)
        {
            throw;
        }

        // Retrieve socket information
        socklen_t socketSize = sizeof(sockAddr);
        ret = getsockname(m_socketFd, (sockaddr*)&sockAddr, &socketSize);
        m_bindedPort = ntohs(sockAddr.sin_port);

        static_assert(std::is_move_constructible_v<UDPSocket>);
    }

    ~UDPSocket()
    {
        if (m_socketFd >= 0)
        {
            close(m_socketFd);
            m_socketFd = -1;
        }
    }

    bool setReceiveTimeout(time_t seconds)
    {
        if (m_socketFd < 0)
        {
            return false;
        }

        timeval tv{};
        tv.tv_sec = seconds;

        if (setsockopt(m_socketFd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) != 0)
        {
            throw;
        }

        return true; 
    }

    int socketFd() const
    {
        return m_socketFd;
    }

    uint16_t bindedPort() const
    {
        return m_bindedPort;
    }

    // Might throw core::exception::SocketException
    ssize_t recv(uint8_t* buffer, size_t size, IPEndpoint& sender)
    {
        socklen_t socketSize = sender.size();
        ssize_t ret = recvfrom(m_socketFd, buffer, size, 0, sender.get(), &socketSize);

        if (ret == -1 && (errno != EWOULDBLOCK && errno != EINTR))
        {
            printf("%d\n", errno);
            throw core::exception::SocketException{};
        }
        
        return ret;
    }

    // Might throw core::exception::SocketException
    ssize_t send(const uint8_t* buffer, size_t size, const IPEndpoint& receiver)
    {
        ssize_t ret = sendto(m_socketFd, buffer, size, 0, receiver.getConst(), receiver.size());

        if (ret == -1 && errno != EINTR)
        {
            throw core::exception::SocketException{};
        }
        
        return ret;
    }

private:
    uint16_t m_bindedPort;
    int m_socketFd;
};