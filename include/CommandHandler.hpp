#pragma once

#include "Client.hpp"

#define ERR_TOOMANYTARGETS 4

class Server;

typedef struct kickData
{
    std::string cmd;
    std::vector<std::string>channels;
    std::vector<std::string>users;
    std::string reason;
}kickData;

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

        //fonction utilitaire
        void BroadcastToChannel(std::string channelName, Client &cl, std::string msg);

        //gestion de channels
        std::pair<std::vector<std::string>, std::vector<std::string>> fillListCmdInvite(std::istringstream &iss);
        const int checkCmdInvite(std::string chanName, std::string userName, Client &cl);
        void cmdInvite(int fd, std::istringstream &iss, Client &cl);

        int cmdJoinAnExistingChannel(int fd, Client &cl, int targetId, std::string password);
        std::pair<std::vector<std::string>, std::vector<std::string>> fillListCmdJoin(std::istringstream &iss);
        void cmdJoin(int fd, std::istringstream &iss, Client &cl);

        void kickOneChannelMultiUsers(int fd, Client& cl, kickData& data);
        void kickMultiChannelsOneUser(int fd, Client& cl, kickData& data);
        void kickChannelsUsersPair(int fd, Client& cl, kickData& data);
        void parseKick(std::istringstream &iss, std::vector<std::string> &channels,
        std::vector<std::string> &users, std::string &reason);
        bool checkInKick(std::string channelName, std::string targetNick,
        Client& cl, std::string cmd, int fd);
        std::string msgKick(std::string reason, Client &cl, std::string Channel,
            std::string user);
        void cmdkick(int fd, std::istringstream &iss, Client &cl);

        //messagerie
        struct WhichTarget
		{
			enum Type { USER, CHANNEL } type;
			int id; // fd client ou id channel
		};
        std::vector<std::string> parsePrivMsg(std::istringstream &iss,
            Client &cl, std::string &userMsg);
		std::vector<WhichTarget> validatePrivMsg(Client &cl,
            std::vector<std::string> &targets);
		void sendPrivMsg(Client &cl, std::vector<std::string> &targets,
            std::vector<WhichTarget> &whichTarget, std::string &userMsg);
        void cmdPrivMsg(int fd, std::istringstream &iss, Client &cl);

        //quit
        std::string parsequit(std::istringstream &iss, Client &cl);
        void removeFromChannels(int fd, Client &cl, const std::string& quitMsg);
        void cmdQuit(int fd, std::istringstream &iss, Client &cl);

    public:

        CommandHandler(Server* server);
        void registerCommands();
        void executeCommand(const std::string& cmdLine, int fd, Client& cl);
};
