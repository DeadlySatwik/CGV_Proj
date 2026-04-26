#ifndef RLTRAFFICCLIENT_H
#define RLTRAFFICCLIENT_H

#include <string>

class RLTrafficClient
{
public:
    RLTrafficClient(const std::string &host = "127.0.0.1", int port = 5555);
    ~RLTrafficClient();

    // Returns action in [0,3] or -1 if server is unavailable.
    int requestAction(int carsNorth,
                      int carsSouth,
                      int carsEast,
                      int carsWest,
                      int currentPhase,
                      float greenDuration,
                      float reward);

private:
    std::string host;
    int port;
    int socketFd;
    bool connected;

    bool connectIfNeeded();
    void closeSocket();
};

#endif // RLTRAFFICCLIENT_H
