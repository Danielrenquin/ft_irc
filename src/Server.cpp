#include "Server.hpp"

Server::Server(int port) : _port(port)
{
    if(port < 1 || port > 65535)
        throw std::invalid_argument("Port must be between 1 and 65535");
    _serverAddr = sockaddr_in();
}

Server::~Server()
{
    if(_serverSocket != -1)
        close(_serverSocket);
}

void Server::createSocket()
{
    _serverSocket = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);

    if(_serverSocket == -1)
    {
        throw std::runtime_error("socket failed");
    }
}

void Server::setSocketData()
{
    _serverAddr.sin_family = AF_INET;
    _serverAddr.sin_port = htons(_port);
    _serverAddr.sin_addr.s_addr = INADDR_ANY;
}

void Server::bindToPort()
{
    socklen_t lengthOfServerAddr;
    lengthOfServerAddr = sizeof(_serverAddr);

    _bound = bind(_serverSocket, (struct sockaddr*) &_serverAddr, lengthOfServerAddr);

    if(_bound == -1)
    {
        throw std::runtime_error("bounding failled try with a ohter port");
    }
}

void Server::startListening()
{
    _listening = listen(_serverSocket, PENDING_QUEUE_MAXLENGTH);
    if(_listening == -1)
        throw std::runtime_error("listen failed");

    while (true)
    {
        struct sockaddr_in clientAddr;
        socklen_t clientLen = sizeof(clientAddr);

        int clientSocket = accept(_serverSocket, (struct sockaddr *)&clientAddr, &clientLen);
        if (clientSocket == -1)
        {
            //std::cerr << "accept failed" << std::endl;
            continue; // ne quitte pas, continue à écouter
        }
        std::cout << "New client connected!" << std::endl;
    }

}
