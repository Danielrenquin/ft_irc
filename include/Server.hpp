#ifndef SERVER_HPP
#define SERVER_HPP

#include <sys/socket.h>
#include <stdexcept>
#include <netinet/in.h>
#include <string>
#include <unistd.h>
#include <iostream>
#define PENDING_QUEUE_MAXLENGTH 10
#include <cerrno>

class Server
{
    private:
        int _serverSocket;
        int _port; // verifier la validitée du port 1 à 65535
        std::string _password; // à ajouter plus tard au constructeur du server
        struct sockaddr_in _serverAddr;
        int _bound;
        int _listening;
    public:
        Server(int port);
        ~Server();

        void createSocket();
        void setSocketData();
        void bindToPort();
        void startListening();
};

#endif