#include "RLTrafficClient.h"

#include <sstream>

#if defined(_WIN32)

RLTrafficClient::RLTrafficClient(const std::string &hostValue, int portValue)
    : host(hostValue), port(portValue), socketFd(-1), connected(false)
{
}

RLTrafficClient::~RLTrafficClient()
{
}

bool RLTrafficClient::connectIfNeeded()
{
    return false;
}

void RLTrafficClient::closeSocket()
{
}

int RLTrafficClient::requestAction(int carsNorth,
                                   int carsSouth,
                                   int carsEast,
                                   int carsWest,
                                   int currentPhase,
                                   float greenDuration,
                                   float reward)
{
    (void)carsNorth;
    (void)carsSouth;
    (void)carsEast;
    (void)carsWest;
    (void)currentPhase;
    (void)greenDuration;
    (void)reward;
    return -1;
}

#else

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

RLTrafficClient::RLTrafficClient(const std::string &hostValue, int portValue)
    : host(hostValue), port(portValue), socketFd(-1), connected(false)
{
}

RLTrafficClient::~RLTrafficClient()
{
    closeSocket();
}

bool RLTrafficClient::connectIfNeeded()
{
    if (connected)
    {
        return true;
    }

    socketFd = socket(AF_INET, SOCK_STREAM, 0);
    if (socketFd < 0)
    {
        return false;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons((uint16_t)port);

    if (inet_pton(AF_INET, host.c_str(), &serverAddr.sin_addr) <= 0)
    {
        closeSocket();
        return false;
    }

    if (connect(socketFd, (sockaddr*)&serverAddr, sizeof(serverAddr)) < 0)
    {
        closeSocket();
        return false;
    }

    connected = true;
    return true;
}

void RLTrafficClient::closeSocket()
{
    if (socketFd >= 0)
    {
        close(socketFd);
    }
    socketFd = -1;
    connected = false;
}

int RLTrafficClient::requestAction(int carsNorth,
                                   int carsSouth,
                                   int carsEast,
                                   int carsWest,
                                   int currentPhase,
                                   float greenDuration,
                                   float reward)
{
    if (!connectIfNeeded())
    {
        return -1;
    }

    std::ostringstream oss;
    oss << carsNorth << ' ' << carsSouth << ' ' << carsEast << ' ' << carsWest << ' '
        << currentPhase << ' ' << greenDuration << ' ' << reward << '\n';
    const std::string payload = oss.str();

    const ssize_t sent = send(socketFd, payload.c_str(), payload.size(), 0);
    if (sent < 0)
    {
        closeSocket();
        return -1;
    }

    char buffer[64];
    const ssize_t recvd = recv(socketFd, buffer, sizeof(buffer) - 1, 0);
    if (recvd <= 0)
    {
        closeSocket();
        return -1;
    }

    buffer[recvd] = '\0';
    std::istringstream iss(buffer);
    int action = -1;
    iss >> action;

    if (action < 0 || action > 3)
    {
        return -1;
    }

    return action;
}

#endif
