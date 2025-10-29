#pragma once

#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <sstream>
#include <set>
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
class Channel;
struct ClientContext;

class Server
{
    private:
        int _port;
        int _serverSocket;
		size_t _pfdsIndex; // Index principale de la première boucle for
		char *_password;
        std::map<int, Client> _clients;
		std::map<int, ClientContext> _contexts;
		std::map<int, Channel> _channelList;
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
			WAIT_PASS,
			WAIT_NICK,
			WAIT_USER,
			AUTHENTICATED
		};

        void initSocket(char **argv); // création + bind via getaddrinfo
        void startListening(); // accepte les connexions entrantes

        void addClient(int fd);
        void removeClient();
        //int login(int clientIndex);
        void handleClientData();
        void handleConnections();
		void acceptNewClient();
		void broadcastMessage(int senderFd, const std::string &msg);

		// Getter & Setter
		void setPassword(char *password);
		const int getClientFdByNickname(std::string nickname);
		const int getChannelIdByName(std::string &channelName);

		void registerClient(int fd, ClientContext &ctx, Client &cl);

		// CMD
		void isCmd(std::string &msg, int fd, std::istringstream &iss, Client &cl);
		void cmdQuit(int fd, Client &cl);
		void cmdJoin(int fd, std::istringstream &iss, Client &cl);
		void cmdPrivMsg(int fd, std::istringstream &iss, Client &cl);
		void cmdPass(int fd, std::istringstream &iss, ClientContext &ctx, Client &cl);
		void cmdUser(int fd, std::istringstream &iss, ClientContext &ctx, Client &cl);
		void cmdNick(int fd, std::istringstream &iss, ClientContext &ctx, Client &cl);
		void cmdJoinAnExistingChannel(int fd, Client &cl, int targetId);
		void cmdkick(int fd, std::istringstream &iss, Client &cl);

		std::string trim(const std::string &s);

		bool isUserAlreadyInChannel(int fd, int targetId);
		bool isUserInAnyChannel(int fd);


};

//separer les fonctions pour operateur/client
//ajouter le passeword
//inserer le containeur de poll dans un containeur de clients
