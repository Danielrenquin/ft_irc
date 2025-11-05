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

    if(cmdLine == "DEBUG")
    {
        std::map<int, Channel>& _channelList = _server->getChannelList();
        printChannelMap(_channelList, "Debug");
    }
    if(it != _commands.end())
    {
        (this->*(it->second))(fd, iss, cl);
    }
    else
    {
        ErrorHandler::unknownCommand(cl, cmdLine);
    }
}

void CommandHandler::BroadcastToChannel(std::string channelName, Client &cl, std::string msg)
{
    std::map<int, Client>& clients = _server->getClients();
    std::map<int, Channel>& channels = _server->getChannelList();

    if(channelName[0] == '#' || channelName[0] == '$')
        channelName = channelName.substr(1);
    int channelId = _server->getChannelIdByName(channelName);
    Channel &channel = channels[channelId];
    std::set<int> userList = channel.getUserList();

    for (std::set<int>::iterator it = userList.begin(); it != userList.end(); ++it)
    {
        int userFd = *it;
        clients[userFd].sendMessage(msg);
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
}

//crée deux vecteur de string afin de gérer les commandes a argument multiple
void CommandHandler::parseKick(std::istringstream &iss, std::vector<std::string> &channels,
std::vector<std::string> &users, std::string &reason)
{
    std::string channelList;
    std::string userList;

    iss >> channelList >> userList;

    std::getline(iss, reason);
    reason = _server->trim(reason);

    std::stringstream sschan(channelList);
    std::string chan;
    while (std::getline(sschan, chan, ','))
        channels.push_back(_server->trim(chan));

    std::stringstream ssuser(userList);
    std::string user;
    while (std::getline(ssuser, user, ','))
        users.push_back(_server->trim(user));
}

bool CommandHandler::checkInKick(std::string channelName, std::string targetNick,
Client& cl, std::string cmd, int fd)
{
    if(channelName[0] != '#' && channelName[0] != '$')
    {
        ErrorHandler::ERR_BADCHANMASK(cl, channelName);
        return false;
    }
    std::string nameWithoutHash = channelName.substr(1);
    int channelId = _server->getChannelIdByName(nameWithoutHash);
    if(channelId == -1)
    {
        ErrorHandler::noSuchChannel(cl, channelName);
        return false;
    }
    std::map<int, Channel>& channels = _server->getChannelList();
    Channel &channel = channels[channelId];
    if(!_server->isUserAlreadyInChannel(fd, channelId))
    {
        ErrorHandler::notOnChannel(cl, channelName);
        return false;
    }
    if (!channel.isOperator(fd))
    {
        ErrorHandler::chanOPrivsNeeded(cl, channelName);
        return false;
    }
    int targetFd = _server->getClientFdByNickname(targetNick);
    if (targetFd == 0 || !_server->isUserAlreadyInChannel(targetFd, channelId))
    {
        ErrorHandler::userNotInChannel(cl, targetNick, channelName);
        return false;
    }
    return true;
}

std::string CommandHandler::msgKick(std::string reason, Client &cl, std::string channel,
std::string user)
{
    if(reason.empty())
        reason = cl.getNickname();

    std::string kickMsg = ":" + cl.getNickname() + "!" + cl.getUsername() + "@localhost KICK "
                        + channel + " " + user + " :" + reason + "\r\n";
    return kickMsg;
}

/*void CommandHandler::cmdkick(int fd, std::istringstream& iss, Client& cl)
{
    std::string cmd = iss.str();
    std::vector<std::string> Channels;
    std::vector<std::string> Users;
    std::string reason;

    // --- 1. Parsing ---
    parseKick(iss, Channels, Users, reason);

    if (Channels.empty() || Users.empty())
    {
        ErrorHandler::needMoreParams(cl, cmd);
        return;
    }

    std::map<int, Channel>& channels = _server->getChannelList();

    // --- 2. RFC 1459 logic ---
    // Case 1 : 1 channel, multiple users
    if (Channels.size() == 1 && Users.size() >= 1)
    {
        std::string chan = Channels[0];
        std::string nameWithoutHash = (chan[0] == '#' || chan[0] == '$') ? chan.substr(1) : chan;
        int channelId = _server->getChannelIdByName(nameWithoutHash);

        if (channelId == -1)
        {
            ErrorHandler::noSuchChannel(cl, chan);
            return;
        }

        for (size_t j = 0; j < Users.size(); ++j)
        {
            if (checkInKick(chan, Users[j], cl, cmd, fd))
            {
                std::string msg = msgKick(reason, cl, chan, Users[j]);
                BroadcastToChannel(chan, cl, msg);

                int targetFd = _server->getClientFdByNickname(Users[j]);
                if (targetFd > 0)
                    channels[channelId].removeUser(targetFd);
            }
        }
    }
    // Case 2 : multiple channels, 1 user
    else if (Channels.size() >= 1 && Users.size() == 1)
    {
        std::string user = Users[0];
        for (size_t i = 0; i < Channels.size(); ++i)
        {
            std::string chan = Channels[i];
            std::string nameWithoutHash = (chan[0] == '#' || chan[0] == '$') ? chan.substr(1) : chan;
            int channelId = _server->getChannelIdByName(nameWithoutHash);

            if (channelId == -1)
            {
                ErrorHandler::noSuchChannel(cl, chan);
                continue;
            }

            if (checkInKick(chan, user, cl, cmd, fd))
            {
                std::string msg = msgKick(reason, cl, chan, user);
                BroadcastToChannel(chan, cl, msg);

                int targetFd = _server->getClientFdByNickname(user);
                if (targetFd > 0)
                    channels[channelId].removeUser(targetFd);
            }
        }
    }
    // Case 3 : same number of channels and users
    else if (Channels.size() == Users.size())
    {
        for (size_t i = 0; i < Channels.size(); ++i)
        {
            std::string chan = Channels[i];
            std::string user = Users[i];

            std::string nameWithoutHash = (chan[0] == '#' || chan[0] == '$') ? chan.substr(1) : chan;
            int channelId = _server->getChannelIdByName(nameWithoutHash);

            if (channelId == -1)
            {
                ErrorHandler::noSuchChannel(cl, chan);
                continue;
            }

            if (checkInKick(chan, user, cl, cmd, fd))
            {
                std::string msg = msgKick(reason, cl, chan, user);
                BroadcastToChannel(chan, cl, msg);

                int targetFd = _server->getClientFdByNickname(user);
                if (targetFd > 0)
                    channels[channelId].removeUser(targetFd);
            }
        }
    }
    // Case 4 : invalid argument count
    else
    {
        // RFC 1459 §4.2.8 — behavior undefined → reject command
        ErrorHandler::needMoreParams(cl, cmd);
        return;
    }
}*/

void CommandHandler::kickOneChannelMultiUsers(int fd, Client& cl, kickData& data)
{
    std::map<int, Channel>& channelList = _server->getChannelList();
    std::string chan = data.channels[0];
    std::string nameWithoutHash = (chan[0] == '#' || chan[0] == '$') ? chan.substr(1) : chan;

    int channelId = _server->getChannelIdByName(nameWithoutHash);
    if (channelId == -1)
    {
        ErrorHandler::noSuchChannel(cl, chan);
        return;
    }

    for (size_t j = 0; j < data.users.size(); ++j)
    {
        if (checkInKick(chan, data.users[j], cl, data.cmd, fd))
        {
            std::string msg = msgKick(data.reason, cl, chan, data.users[j]);
            BroadcastToChannel(chan, cl, msg);

            int targetFd = _server->getClientFdByNickname(data.users[j]);
            if (targetFd > 0)
                channelList[channelId].removeUser(targetFd);
        }
    }
}

void CommandHandler::kickMultiChannelsOneUser(int fd, Client& cl, kickData& data)
{
    std::map<int, Channel>& channelList = _server->getChannelList();
    std::string user = data.users[0];

    for (size_t i = 0; i < data.channels.size(); ++i)
    {
        std::string chan = data.channels[i];
        std::string nameWithoutHash = (chan[0] == '#' || chan[0] == '$') ? chan.substr(1) : chan;

        int channelId = _server->getChannelIdByName(nameWithoutHash);
        if (channelId == -1)
        {
            ErrorHandler::noSuchChannel(cl, chan);
            continue;
        }

        if (checkInKick(chan, user, cl, data.cmd, fd))
        {
            std::string msg = msgKick(data.reason, cl, chan, user);
            BroadcastToChannel(chan, cl, msg);

            int targetFd = _server->getClientFdByNickname(user);
            if (targetFd > 0)
                channelList[channelId].removeUser(targetFd);
        }
    }
}

void CommandHandler::kickChannelsUsersPair(int fd, Client& cl, kickData& data)
{
    std::map<int, Channel>& channelList = _server->getChannelList();

    for (size_t i = 0; i < data.channels.size(); ++i)
    {
        std::string chan = data.channels[i];
        std::string user = data.users[i];
        std::string nameWithoutHash = (chan[0] == '#' || chan[0] == '$') ? chan.substr(1) : chan;

        int channelId = _server->getChannelIdByName(nameWithoutHash);
        if (channelId == -1)
        {
            ErrorHandler::noSuchChannel(cl, chan);
            continue;
        }

        if (checkInKick(chan, user, cl, data.cmd, fd))
        {
            std::string msg = msgKick(data.reason, cl, chan, user);
            BroadcastToChannel(chan, cl, msg);

            int targetFd = _server->getClientFdByNickname(user);
            if (targetFd > 0)
                channelList[channelId].removeUser(targetFd);
        }
    }
}

void CommandHandler::cmdkick(int fd, std::istringstream& iss, Client& cl)
{
    kickData data;
    data.cmd = iss.str();
    parseKick(iss, data.channels, data.users, data.reason);

    if (data.channels.empty() || data.users.empty())
    {
        ErrorHandler::needMoreParams(cl, data.cmd);
        return;
    }
    if (data.channels.size() == 1 && data.users.size() >= 1)
        kickOneChannelMultiUsers(fd, cl, data);
    else if (data.channels.size() >= 1 && data.users.size() == 1)
        kickMultiChannelsOneUser(fd, cl, data);
    else if (data.channels.size() == data.users.size())
        kickChannelsUsersPair(fd, cl, data);
    else
        ErrorHandler::needMoreParams(cl, data.cmd);
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
