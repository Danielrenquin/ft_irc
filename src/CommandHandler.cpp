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
    _commands["JOIN"] = &CommandHandler::cmdJoin; //JOIN #salon avec cette commande le channel s' appel #salon et pas salon
    _commands["KICK"] = &CommandHandler::cmdkick;
    _commands["PRIVMSG"] = &CommandHandler::cmdPrivMsg;
    _commands["QUIT"] = &CommandHandler::cmdQuit;
    _commands["INVITE"] = &CommandHandler::cmdInvite;
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

int CommandHandler::cmdJoinAnExistingChannel(int fd, Client &cl, int targetId, std::string password)
{
    std::map<int, Channel>& channelList = _server->getChannelList();
    Channel &ch = channelList[targetId];
	std::string formattedMsg = ":" + cl.getNickname() + "!" + cl.getUsername() + "@" + cl.getHostname() + " JOIN " + " :" + ch.getName();

    if (!_server->isUserAlreadyInChannel(fd, targetId))
    {
        if(!ch.getPassword().empty())
        {
            if(password == ch.getPassword())
            {
                ch.sendMessage(formattedMsg);
                ch.addUser(fd);
                return (0);
            }
            else
            {
                ErrorHandler::badChannelKey(cl, ch.getName());
                return(-1);
            }
        }
        ch.sendMessage(formattedMsg);
        ch.addUser(fd);
    }
    else
    {
        ErrorHandler::userOnChannel(cl, cl.getNickname(), ch.getName());
        return(-1);
    }
    return(0);
}

std::pair<std::vector<std::string>, std::vector<std::string>> CommandHandler::fillListCmdJoin(std::istringstream &iss)
{
    std::string channelsToken;
    std::string keysToken;

    iss >> channelsToken;
    iss >> keysToken;

    //split channels
    std::vector<std::string> channelList;
    std::stringstream cstream(channelsToken);
    std::string chan;
    while (std::getline(cstream, chan, ','))
        channelList.push_back(chan);

    //split keys
    std::vector<std::string> passList;
    if(!keysToken.empty())
    {
        std::stringstream kstream(keysToken);
        std::string key;
        while (std::getline(kstream, key, ','))
            passList.push_back(key);
    }
    return {channelList, passList};
}

void CommandHandler::cmdJoin(int fd, std::istringstream &iss, Client &cl)
{
    static int id = 0;
    std::map<int, Channel>& _channelList = _server->getChannelList();
    std::pair<std::vector<std::string>, std::vector<std::string>> data = fillListCmdJoin(iss);
    std::vector<std::string> channelList = data.first;
	std::vector<std::string> passList = data.second;

    for(size_t i = 0; i < channelList.size(); i++)
    {
        std::string &channel = channelList[i];
        std::string pass = (i < passList.size()) ? passList[i] : "";

        // check prefix
        if (channel.empty() ||
        (channel[0] != '#' && channel[0] != '&' &&
        channel[0] != '!' && channel[0] != '+'))
        {
            ErrorHandler::ERR_BADCHANMASK(cl, channel);
            continue;
        }

        // check channel exist
        int targetId = _server->getChannelIdByName(channel);
        if(targetId != -1)
        {
            //a réfléchir en cas de out of range avec la cmd join
            //std::string password = (i < passList.size()) ? passList[i] : "";
            std::string password = passList.empty() ? "" : passList[i];
            if(cmdJoinAnExistingChannel(fd, cl, targetId, password) == -1)
                return;
            continue;
        }
         // Create channel
        _channelList[id] = passList.empty()
            ? Channel(id, channel, fd, fd)
            : Channel(id, channel, pass, fd, fd);

        Channel &ch = _channelList[id];
        ch.addOperator(fd);
        ch.addUser(fd);
        id++;
    }
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

std::pair<std::vector<std::string>, std::vector<std::string>> CommandHandler::fillListCmdInvite(std::istringstream &iss)
{
    std::string channelsToken;
	std::string nameToken;

	iss >> channelsToken;
	iss >> nameToken;

	// split channels
	std::vector<std::string> channelList;
	std::stringstream chanStream(channelsToken);
	std::string chan;
	while (std::getline(chanStream, chan, ','))
    channelList.push_back(chan);

	// split keys
	std::vector<std::string> nameList;
	if (!nameToken.empty())
	{
		std::stringstream nameStream(nameToken);
		std::string name;
		while (std::getline(nameStream, name, ','))
        nameList.push_back(name);
	}
	return {channelList, nameList};
}

const int CommandHandler::checkCmdInvite(std::string chanName, std::string userName, Client &cl)
{
    if (userName.empty() || _server->getClientFdByNickname(userName) == -1)
    {
        cl.sendMessage("ERROR :User not found\r\n");
        return (-1);
    }

    if (chanName.empty() ||
    (chanName[0] != '#' && chanName[0] != '&' &&
        chanName[0] != '!' && chanName[0] != '+'))
        {
        cl.sendMessage("ERROR :Need a prefix\r\n");
        return (-1);
    }
    return (0);
}
// TODO: More testing
void CommandHandler::cmdInvite(int fd, std::istringstream &iss, Client &cl)
{
    std::string chanName;
	std::string userName;

	iss >> userName;
	iss >> chanName;

    if (checkCmdInvite(chanName, userName, cl) == -1)
    {

        // Check channel exists
        int targetId = _server->getChannelIdByName(chanName);
        if (targetId != -1)
        {
            if (_server->isUserAlreadyInChannel(_server->getClientFdByNickname(userName), targetId) == true)
                cl.sendMessage("ERROR :User already in this channel\r\n");
            return ;
        }
        if (_server->isUserAlreadyInChannel(fd, targetId) == true && _server->isUserOperator(fd, targetId) == true)
        {
            Channel *ch = _server->getChannelById(targetId);
            Client *targetCl = _server->getClientByFd(_server->getClientFdByNickname(userName));
            std::string formattedMsg = ":" + cl.getNickname() + "!" + cl.getUsername() + "@" + cl.getHostname() + " JOIN " + " :" + chanName;
            targetCl->sendMessage(formattedMsg);
            ch->inviteUser(_server->getClientFdByNickname(userName));
        }
        else
        {
            cl.sendMessage("ERROR :You're not an operator\r\n");
            return ;
        }
    }
    else
    {
        cl.sendMessage("ERROR :No such channel\r\n");
        return ;
    }
}

std::vector<std::string> CommandHandler::parsePrivMsg(std::istringstream &iss,
    Client &cl, std::string &userMsg)
    {
        std::vector<std::string> targets;
        std::string target;
        int i = 0;

        while (std::getline(iss, target, ','))
        {
            size_t pos = target.find(":");
            if(pos != std::string::npos)
            {
                userMsg = target;
                target = target.substr(0, pos);
                userMsg = userMsg.substr(pos, userMsg.size() - pos);
                if (userMsg[0] == ':')
			    userMsg.erase(0, 1);
            }
            while ((pos = target.find(' ')) != std::string::npos)
            {
                target.erase(pos, 1);
            }
            targets.push_back(target);
            if(i > ERR_TOOMANYTARGETS)
            {
                ErrorHandler::tooManyTargets(cl, target);
                return {};
            }
            i++;
        }
        return(targets);
    }

    std::vector<CommandHandler::WhichTarget> CommandHandler::validatePrivMsg(Client &cl, std::vector<std::string> &targets)
    {
        std::vector<WhichTarget> whichTarget;

        for (size_t i = 0; i < targets.size(); i++)
        {
            if (targets[i].empty())
            {
                ErrorHandler::noRecipient(cl, cl.getNickname());
                return {};
		}

		WhichTarget wt;
		size_t pos = targets[i].find("#");
		if (pos != std::string::npos)
		{
            wt.type = WhichTarget::CHANNEL;
			wt.id = _server->getChannelIdByName(targets[i]);
			if (wt.id < 0)
			{
                ErrorHandler::noSuchChannel(cl, targets[i]);
				return {};
			}
		}
		else
		{
            wt.type = WhichTarget::USER;
			wt.id = _server->getClientFdByNickname(targets[i]);
			if (wt.id < 0)
			{
                ErrorHandler::noSuchNick(cl, cl.getNickname());
				return {};
			}
		}
		whichTarget.push_back(wt);
	}
	return (whichTarget);
}


void CommandHandler::sendPrivMsg(Client &cl, std::vector<std::string> &targets, std::vector<WhichTarget> &whichTarget, std::string &userMsg)
{
    std::map<int, Channel>& channelList = _server->getChannelList();
    std::map<int, Client>& clients = _server-> getClients();
	std::string formattedMsg = ":" + cl.getNickname() + "!" + cl.getUsername() + "@" + cl.getHostname() + " PRIVMSG ";

    for (size_t i = 0; i < whichTarget.size(); i++)
    {
        WhichTarget wt = whichTarget[i];
        int targetFd = wt.id;

        std::string fullMsg = formattedMsg + targets[i] + " :" + userMsg + "\r\n";

		if (wt.type == WhichTarget::CHANNEL)
		{
            Channel &targetCh = channelList[targetFd];
			targetCh.sendMessageAvoidingSender(fullMsg, cl.getFd());
		}
		else
		{
            Client &targetCl = clients[targetFd];
			targetCl.sendMessage(fullMsg);
		}
    }
}

void CommandHandler::cmdPrivMsg(int fd, std::istringstream &iss, Client &cl)
{
    std::vector<std::string> targets;
    std::vector<CommandHandler::WhichTarget> whichTarget;
    std::string userMsg;
    std::string target;

    // Parsing des cibles
    targets = parsePrivMsg(iss, cl, userMsg);
    if (targets.empty())
    return;

    // Validation des cibles (users/channels)
    whichTarget = validatePrivMsg(cl, targets);
    if (whichTarget.empty())
    return;

    // Envoi du message aux cibles
    sendPrivMsg(cl, targets, whichTarget, userMsg);
}

std::string CommandHandler::parsequit(std::istringstream &iss, Client &cl)
{
    std::string quitMsg;
    std::getline(iss, quitMsg);

    if(!quitMsg.empty() && quitMsg[0] == ' ')
    quitMsg.erase(0, 1);
    if(!quitMsg.empty() && quitMsg[0] == ':')
    quitMsg.erase(0, 1);
    if(quitMsg.empty())
    quitMsg = "Client disconnected";

    std::string formattedMsg = ":" + cl.getNickname() + "!" + cl.getUsername() +
    "@" + cl.getHostname() + " QUIT :" + quitMsg + "\r\n";

    return formattedMsg;
}

void CommandHandler::removeFromChannels(int fd, Client &cl, std::string quitMsg)
{
    std::map<int, Channel>& ChannelList = _server->getChannelList();
    std::set<int> Users;
    std::set<int> Operator;
    int flag = 0;

    for(std::map<int, Channel>::iterator it = ChannelList.begin(); it != ChannelList.end(); ++it)
    {
        Users = it->second.getUserList();
        if (Users.find(fd) != Users.end())
        {
            it->second.sendMessageAvoidingSender(quitMsg, fd);
            it->second.removeUser(fd);
            Operator = it->second.getOperators();
            if(Operator.find(fd) != Operator.end())
            {
                it->second.removeOperator(fd);
            }
        }
        if(it->second.getUserList().empty() && it->second.getOperators().empty())
        {
            ChannelList.erase(it);
            flag = 1;
            break ;
        }
        else if(!it->second.getUserList().empty() && it->second.getOperators().empty())
        {
            it->second.addOperator(*Users.begin());
        }
    }
    if (flag == 1)
        removeFromChannels(fd, cl, quitMsg);
}

void CommandHandler::cmdQuit(int fd, std::istringstream &iss, Client &cl)
{
    std::string quitMsg;
    quitMsg = parsequit(iss, cl);
    removeFromChannels(fd, cl, quitMsg);
    _server->removeClient(fd);
}
