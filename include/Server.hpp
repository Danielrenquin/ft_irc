#pragma once

#include <algorithm>
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
struct ClientContext;

class Server
{
    private:
        int _port;
        int _serverSocket;
        std::map<int, Client> _clients;
		std::map<int, ClientContext> _contexts;
        std::vector<pollfd> _pfds;
    public:
        Server(int port);
        ~Server();

		enum eClientInformation
		{
			eUsername,
			eHostname,
			eServername,
			eRealname
		};

		enum ClientState 
		{
			WAIT_USERNAME,
			WAIT_HOSTNAME,
			WAIT_SERVERNAME,
			WAIT_REALNAME,
			AUTHENTICATED
		};

		


        void initSocket(char **argv); // création + bind via getaddrinfo
        void startListening(); // accepte les connexions entrantes

        void addClient(int fd);
        void removeClient(int index);
        int login(int clientIndex);
        void handleClientData(size_t index);
        void handleConnections();

		// Parsing
		int receiveDataFromUser(int clientFd, int clientIndex, char buffer[4][48], int *index);
		int parsingInputs(int clientFd, int clientIndex, char buffer[4][48]);
		int parsingUsername(int clientFd, int clientIndex, char buffer[4][48], int *index);
        int parsingRealname(int clientFd, int clientIndex, char buffer[4][48], int *index);
		void contextSwitch(std::string &msg, ClientContext &ctx, Client &cl, int fd);
		int parsingUsername(std::string &msg, int fd, ClientContext &ctx);
		int parsingRealname(std::string &msg, int fd, ClientContext &ctx);

		void acceptNewClient();
		void broadcastMessage(int senderFd, const std::string &msg);

		std::string trim(const std::string &s);

};

//separer les fonctions pour operateur/client
//ajouter le passeword
//inserer le containeur de poll dans un containeur de clients
