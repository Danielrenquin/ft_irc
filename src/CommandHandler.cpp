#include "CommandHandler.hpp"
#include "Server.hpp"
#include "Channel.hpp"
#include "ErrorHandler.hpp"
#include <iostream>

/*-------------Debug--------------------*/

#include "map"
#include "set"

template <typename K, typename V>
void printChannelMap(const std::map<K, V>& channels, const std::string& title = "Channels list") 
{
    std::cout << "\n=== " << title << " ===" << std::endl;

    if (channels.empty()) 
    {
        std::cout << "(empty)" << std::endl;
        std::cout << "======================" << std::endl;
        return;
    }

    // Parcours de tous les channels
    for (typename std::map<K, V>::const_iterator it = channels.begin(); it != channels.end(); ++it) 
    {
        const V& channel = it->second;

        std::cout << "\nChannel ID: " << it->first<< " | Name: " << channel.getName() << std::endl;

        // --- Affichage des opérateurs ---
        const std::set<int>& operators = channel.getOperators();
        std::cout << "  Operators (" << operators.size() << "): ";
        if (operators.empty()) 
            std::cout << "(none)";
        else 
        {
            for (std::set<int>::const_iterator opIt = operators.begin(); opIt != operators.end(); ++opIt)
                std::cout << *opIt << " ";
        }
        std::cout << std::endl;

        // --- Affichage des utilisateurs ---
        const std::set<int>& users = channel.getUserList();
        std::cout << "  Users (" << users.size() << "): ";
        if (users.empty()) 
            std::cout << "(none)";
        else 
        {
            for (std::set<int>::const_iterator uIt = users.begin(); uIt != users.end(); ++uIt)
            {
                std::cout << *uIt << " ";
            }
        }
        std::cout << std::endl;
    }

    std::cout << "======================" << std::endl;
}

/*------------------------------------------*/

CommandHandler::CommandHandler(Server* server)
{
    _server = server;
}

void CommandHandler::registerCommands()
{
    _commands["JOIN"] = &CommandHandler::cmdJoin;
    _commands["KICK"] = &CommandHandler::cmdkick;
    _commands["PRIVMSG"] = &CommandHandler::cmdPrivMsg;
    _commands["QUIT"] = &CommandHandler::cmdQuit;
}


void CommandHandler::executeCommand(const std::string& cmdLine, int fd, Client& cl)
{
    std::istringstream iss(cmdLine);
    std::string cmd;
    iss >> cmd;
    std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::toupper);

    std::cout << cmd << std::endl;
    std::map<std::string, CommandFunc>::iterator it = _commands.find(cmd);
    if(it != _commands.end())
    {
        (this->*(it->second))(fd, iss, cl);
    }
    else
    {
        ErrorHandler::unknownCommand(cl, cmdLine);
    }
}

void CommandHandler::cmdJoinAnExistingChannel(int fd, Client &cl, int targetId)
{
    std::map<int, Channel>& _channelList = _server->getChannelList();
    Channel &ch = _channelList[targetId];

    if (!_server->isUserAlreadyInChannel(fd, targetId)) 
    {
        ch.addUser(fd);
        std::cout << "[JOIN] Client " << fd << " a rejoint le channel " << ch.getName() << std::endl;
    } 
    else 
    {
        cl.sendMessage("NOTICE : already in this channel\r\n");
    }
}

void CommandHandler::cmdJoin(int fd, std::istringstream& iss, Client& cl) 
{
    static int id = 0;
    std::string userName = cl.getNickname();
    std::string channelName, pass;
    iss >> channelName;
    std::map<int, Channel>& _channelList = _server->getChannelList();

    if (channelName.empty()) 
    {
        cl.sendMessage("ERROR :Channel name required\r\n");
        return;
    }

    int targetId = _server->getChannelIdByName(channelName);
    if (targetId != -1) 
    {
        cmdJoinAnExistingChannel(fd, cl, targetId);
        return;
    }
    
    iss >> pass;
    Channel newChannel;
    if(pass.empty())
    {
        newChannel = Channel(id, channelName, fd, fd);
    }
    else
    {
        newChannel = Channel(id, channelName, pass, fd, fd);
    }
    
    _channelList[id] = newChannel;
    Channel &ch = _channelList[id++];
    ch.addOperator(fd);
    ch.addUser(fd);
    printChannelMap(_channelList, "status");
}

void CommandHandler::cmdkick(int fd, std::istringstream& iss, Client& cl)
{
    std::string cmd = iss.str();
    std::string channelName, targetNick, reason;
    iss >> channelName >> targetNick;
    std::getline(iss, reason);

    channelName = _server->trim(channelName);
    targetNick = _server->trim(targetNick);
    reason = _server->trim(reason);

    if (channelName.empty() || targetNick.empty()) 
    {
        ErrorHandler::needMoreParams(cl, cmd);
        return;
    }
    if (channelName[0] != '#' && channelName[0] != '$') 
    {
        ErrorHandler::ERR_BADCHANMASK(cl, channelName);
        return;
    }
    std::string nameWithoutHash = channelName.substr(1);
    int channelId = _server->getChannelIdByName(nameWithoutHash); // enlever le #
    if (channelId == -1) 
    {
        ErrorHandler::noSuchChannel(cl, channelName);
        return;
    }
    std::map<int, Channel>& channels = _server->getChannelList();
    Channel &channel = channels[channelId];
    if (!_server->isUserAlreadyInChannel(fd, channelId)) 
    {
        ErrorHandler::notOnChannel(cl, channelName);
        return;
    }
    if (!channel.isOperator(fd)) 
    {
        ErrorHandler::chanOPrivsNeeded(cl, channelName);
        return;
    }
    int targetFd = _server->getClientFdByNickname(targetNick);
    if (targetFd == 0 || !_server->isUserAlreadyInChannel(targetFd, channelId)) 
    {
        ErrorHandler::userNotInChannel(cl, targetNick, channelName);
        return;
    }
    if (reason.empty())
        reason = cl.getNickname();

    std::string kickMsg = ":" + cl.getNickname() + "!" + cl.getUsername() + "@localhost KICK " +
                          channelName + " " + targetNick + " :" + reason + "\r\n";

    // Envoi à tous les utilisateurs du channel
    std::map<int, Client>& clients = _server->getClients();
    for (int userFd : channel.getUserList()) 
    {
        clients[userFd].sendMessage(kickMsg);
    }
    // Retirer l'utilisateur du channel
    channel.removeUser(targetFd);
    std::cout << "[DEBUG] " << targetNick << " kicked from " << channelName << std::endl;
}

// TODO: implémenter l'envois de message dans les cannaux
void CommandHandler::cmdPrivMsg(int fd, std::istringstream &iss, Client &cl)
{
	std::string targetUser, userMsg;
	iss >> targetUser;
	std::cout << "target user: " << targetUser << std::endl;
	if (targetUser.empty())
	{
		cl.sendMessage("ERROR :Target User required\r\n");
		return ;
	}
	//std::cout << "privmsg test: " << msg << std::endl;
	//std::string targetUser = msg.substr(msg.find(' '), msg.find(':'));
	//std::cout << "privmsg test: " << targetUser << std::endl;
	iss >> userMsg;
	if (userMsg[0] == ':')
		userMsg[0] = '\0';
	std::cout << "user msg: " << userMsg << std::endl;

	int targetFd = _server->getClientFdByNickname(targetUser);
	if (targetFd == 0)
	{
		cl.sendMessage("ERROR :Target User not found in the server\r\n");
		return ;
	}
    std::map<int, Client> clients = _server->getClients();
	Client &clTarget = clients[targetFd];

	if (targetFd > 0) // User existe
	{
        //std::cout << "USER FOUND: " << std::endl;
		//std::string userMsg = msg.substr(msg.find(':'), msg.size());
        //write(targetFd, userMsg.c_str(), userMsg.size());
        //write(fd, userMsg.c_str(), userMsg.size());
		clTarget.sendMessage(userMsg);
		cl.sendMessage(userMsg);
	}
}

// TODO: retirer les clients lors de leurs deconnexion ainsi que les channel
void CommandHandler::cmdQuit(int fd, std::istringstream &iss, Client &cl)
{
    std::string quitMsg;
    iss >> quitMsg;
    std::cout << "quit message: " << quitMsg << std::endl;
    std::map<int, Client>& clients = _server->getClients();
	std::map<int, ClientContext>& contexts = _server->getContexts();
    std::cout << "Client " << fd << " disconnected" << std::endl;
    cl.sendMessage("[Server]: Goodbye, " + cl.getUsername() + "\r\n");
    close(fd);
    _server->removeClient();
    clients.erase(fd);
    contexts.erase(fd);
}