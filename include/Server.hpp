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
#include <map>
#include <poll.h>
#include "Client.hpp"

#include <cstring>
#include <arpa/inet.h>

#define PENDING_QUEUE_MAXLENGTH 10
#define MSG_SIZE 512 // a verifier dans le protocole irc ?

//test a réaliser ipv6 et ipv4 normalement ok ?

class Client;

class Server
{
    private:
        int _port;
        int _serverSocket;
        std::map<int, Client> _clients;
        std::vector<pollfd> _pfds;
    public:
        Server(int port);
        ~Server();

        void initSocket(char **argv); // création + bind via getaddrinfo
        void startListening(); // accepte les connexions entrantes

        void addClient(int fd);
        void removeClient(int index);
        int login(int clientIndex);
        void handleClientData(size_t index);
        void handle_connections();

		int receiveDataFromUser(int clientFd, int clientIndex, char buffer[48][48], int *index);
		int parsingInputs(int ClientFd, int clientIndex, char buffer[4][48]);

};

//separer les fonctions pour operateur/client
//ajouter le passeword
//inserer le containeur de poll dans un containeur de clients
