#pragma once

#include "Client.hpp"

class Server;

class CommandHandler
{
    private:

        Server* _server;

        typedef void(CommandHandler::*CommandFunc)(int fd, std::istringstream& iss, Client& cl);
        std::map<std::string, CommandFunc> _commands;

        //authentification 
        //void cmdPass(int fd, std::istringstream &iss, ClientContext &ctx, Client &cl);
		//void cmdUser(int fd, std::istringstream &iss, ClientContext &ctx, Client &cl);
		//void cmdNick(int fd, std::istringstream &iss, ClientContext &ctx, Client &cl);

        //gestion de channels
        void cmdJoinAnExistingChannel(int fd, Client &cl, int targetId);
        void cmdJoin(int fd, std::istringstream& iss, Client &cl);
        void cmdkick(int fd, std::istringstream &iss, Client &cl);
    
        //messagerie
        void cmdPrivMsg(int fd, std::istringstream& iss, Client &cl);

        //autre
        //void cmdQuit(int fd, Client &cl);
    
    public:

        CommandHandler(Server* server);
        void registerCommands();
        void executeCommand(const std::string& cmdLine, int fd, Client& cl);
};
