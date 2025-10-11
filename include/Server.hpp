#pragma once

#include <iostream>
#include <stdexcept>
#include <cstring>
#include <string>
#include <vector>
#include <unistd.h>
#include <netdb.h>       
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>

#define PENDING_QUEUE_MAXLENGTH 10

//test a réaliser ipv6 et ipv4 normalement ok ?

class Server 
{
    private:
        int _port;
        int _serverSocket;
    public:
        Server(int port);
        ~Server();

        void initSocket(char **argv); // création + bind via getaddrinfo
        void startListening(); // accepte les connexions entrantes

};
