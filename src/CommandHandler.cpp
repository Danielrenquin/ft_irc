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
    _commands["TOPIC"] = &CommandHandler::cmdTopic;
	_commands["MODE"] = &CommandHandler::cmdModes;
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

std::pair<std::vector<std::string>, std::vector<std::string> > CommandHandler::fillListCmdJoin(std::istringstream &iss)
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
    return (std::pair<std::vector<std::string>, std::vector<std::string> > (channelList, passList));
}

void CommandHandler::cmdJoin(int fd, std::istringstream &iss, Client &cl)
{
    static int id = 0;
    std::map<int, Channel>& _channelList = _server->getChannelList();
    std::pair<std::vector<std::string>, std::vector<std::string> > data = fillListCmdJoin(iss);
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
            continue ;
        }

        // check channel exist
        int targetId = _server->getChannelIdByName(channel);
        if(targetId != -1)
        {
            //a réfléchir en cas de out of range avec la cmd join
            //std::string password = (i < passList.size()) ? passList[i] : "";
            std::string password = passList.empty() ? "" : passList[i];
            if(cmdJoinAnExistingChannel(fd, cl, targetId, password) == -1)
                return ;
            continue ;
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
        return (false);
    }
    std::string nameWithoutHash = channelName.substr(1);
    int channelId = _server->getChannelIdByName(nameWithoutHash);
    if(channelId == -1)
    {
        ErrorHandler::noSuchChannel(cl, channelName);
        return (false);
    }
    std::map<int, Channel>& channels = _server->getChannelList();
    Channel &channel = channels[channelId];
    if(!_server->isUserAlreadyInChannel(fd, channelId))
    {
        ErrorHandler::notOnChannel(cl, channelName);
        return (false);
    }
    if (!channel.isOperator(fd))
    {
        ErrorHandler::chanOPrivsNeeded(cl, channelName);
        return (false);
    }
    int targetFd = _server->getClientFdByNickname(targetNick);
    if (targetFd == 0 || !_server->isUserAlreadyInChannel(targetFd, channelId))
    {
        ErrorHandler::userNotInChannel(cl, targetNick, channelName);
        return (false);
    }
    return (true);
}

std::string CommandHandler::msgKick(std::string reason, Client &cl, std::string channel,
std::string user)
{
    if(reason.empty())
        reason = cl.getNickname();

    std::string kickMsg = ":" + cl.getNickname() + "!" + cl.getUsername() + "@localhost KICK "
                        + channel + " " + user + " :" + reason + "\r\n";
    return (kickMsg);
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
        return ;
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
            continue ;
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
            continue ;
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
        return ;
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

/*void CommandHandler::cmdPrivMsg(int fd, std::istringstream &iss, Client &cl)
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
}*/

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

std::pair<std::vector<std::string>, std::vector<std::string> > CommandHandler::fillListCmdInvite(std::istringstream &iss)
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
	return (std::pair<std::vector<std::string>, std::vector<std::string> > (channelList, nameList));
}

const int CommandHandler::checkCmdInvite(std::string chanName, std::string userName, Client &cl)
{
    if (userName.empty() || _server->getClientFdByNickname(userName) == -1)
    {
        ErrorHandler::noSuchNick(cl, userName);
        return (-1);
    }

    return (0);
}

void CommandHandler::cmdInvite(int fd, std::istringstream &iss, Client &cl)
{
	std::string chanName;
	std::string userName;

	iss >> userName;
	iss >> chanName;

    if (checkCmdInvite(chanName, userName, cl) == -1)
        return ;

    // Check channel exists
    int targetId = _server->getChannelIdByName(chanName);
    if (targetId != -1)
    {
        if (_server->isUserAlreadyInChannel(_server->getClientFdByNickname(userName), targetId) == true)
        {
            ErrorHandler::userOnChannel(cl, userName, chanName);
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
        else if (_server->isUserOperator(fd, targetId) == false)
        {
            ErrorHandler::notChannelOperator(cl, chanName);
            return ;
        }
        else
        {
            ErrorHandler::notOnChannel(cl, chanName);
            return ;
        }
    }
    else
    {
        ErrorHandler::noSuchChannel(cl, chanName);
        return ;
    }

    // DEBUG
    /*for (std::set<int>::iterator it = ch.getUserList().begin(); it != ch.getUserList().end(); ++it)
    {
        std::cout << "user set: " << *it << std::endl;
    }*/
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
        if(i > MAX_TARGET)
        {
            ErrorHandler::tooManyTargets(cl, target);
            return (std::vector<std::string> ());
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
			return (std::vector<CommandHandler::WhichTarget> ());
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
				return (std::vector<CommandHandler::WhichTarget> ());
			}
		} 
		else 
		{
			wt.type = WhichTarget::USER;
			wt.id = _server->getClientFdByNickname(targets[i]);
			if (wt.id < 0) 
			{
				ErrorHandler::noSuchNick(cl, cl.getNickname());
				return (std::vector<CommandHandler::WhichTarget> ());
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
        return ;

    // Validation des cibles (users/channels)
    whichTarget = validatePrivMsg(cl, targets);
    if (whichTarget.empty())
        return ;

    // Envoi du message aux cibles
    sendPrivMsg(cl, targets, whichTarget, userMsg);
}

int CommandHandler::parsingChannelId(std::istringstream &iss, Client &cl, std::string &channelTarget, std::string command)
{
    iss >> channelTarget;
    if (channelTarget.empty())
    {
        ErrorHandler::notEnoughParameters(cl, command);
        return (-1);
    }

    // Check si le channel target est un channel valide
    int chanId = _server->getChannelIdByName(channelTarget);
    if (chanId < 0) 
    {
        ErrorHandler::noSuchChannel(cl, channelTarget);
        return (-1);
    }
    return (chanId);
}

int CommandHandler::cmdTopicParsingTopicSubjet(int fd, int chanId, Client &cl, Channel *ch, std::string &topic, std::string &channelTarget)
{
    if (!topic.empty() && topic[0] == ' ')
        topic.erase(0, 1);
        
    if (!_server->isUserAlreadyInChannel(fd, chanId)) 
    {
        ErrorHandler::notOnChannel(cl, channelTarget);
        return (-1);
    }
    
    if (topic.empty()) 
    {
        if (ch->getTopic().empty())
            ErrorHandler::noTopicSet(cl, channelTarget);
        else
            cl.sendMessage("332 " + cl.getNickname() + " " + channelTarget + " :" + ch->getTopic());
        return (-1);
    }
    return (0);
}

void CommandHandler::cmdTopic(int fd, std::istringstream &iss, Client &cl)
{
    std::string channelTarget;
    std::string topic;
    
    int chanId = parsingChannelId(iss, cl, channelTarget, "TOPIC");
    if (chanId < 0)
        return ;
    Channel *ch = _server->getChannelById(chanId);

    std::getline(iss, topic);
    if (cmdTopicParsingTopicSubjet(fd, chanId, cl, ch, topic, channelTarget) == -1)
        return ;

    //ch->addMode('t');
    std::set<int> channelModes = ch->getModes();
    if (channelModes.find('t') != channelModes.end())
    {
        if (!_server->isUserOperator(fd, ch->getId()))
        {
            ErrorHandler::notChannelOperator(cl, channelTarget);
            return ;
        }
    }
    
    if (!topic.empty() && topic[0] == ':')
        topic.erase(0, 1);
    
    ch->setTopic(topic);
    std::string msg = ":" + cl.getNickname() + "!" + cl.getUsername() + "@" + cl.getHostname() + " TOPIC " + channelTarget + " :" + topic;
    ch->sendMessage(msg);
}


static int countChar(std::string s, char c) 
{
	int count = 0;

	if (s.empty())
		return (0);

	for (int i = 0; i < (int)s.size(); i++)
	{
		if (s[i] == c) 
			count++;
	}

	return (count);
}


int CommandHandler::cmdModesPreParsing(int fd, std::string &strModes, Client &cl, int &nbrUserRequired)
{
	if (strModes.empty())
	{
		// ERROR mode empty
		return (-1);
	}
	if (strModes.find_first_not_of("+-itkol") != std::string::npos)
	{
		// ERROR char incorrect
		return (-1);
	}
	if (countChar(strModes, 'o') > 3)
	{
		// ERROR too many char
		return (-1);
	}
	unsigned int pos = strModes.find_last_of('o');
	if (pos != std::string::npos)
	{
		for (int i = pos; i > 0; i--)
		{
            if (strModes[i] == 'o')
            {
                nbrUserRequired++;
            }
		}
	}
	return (0);
}

char skipMultipleSign(std::string &strModes, int &i)
{
	bool multipleModes = false;

	while (strModes[i] == '+' || strModes[i] == '-')
	{
		i++;
		multipleModes = true;
	}
	if (multipleModes)
	{
		i--;
	}
	multipleModes = false;
	char sign = strModes[i];

	return (sign);
}

std::pair<unorderedSet<char>, unorderedSet<char> > CommandHandler::cmdModesParsingSets(int fd, std::string &strModes, Client &cl, Channel *ch)
{
    // Replace by unorderedset
	unorderedSet<char> modesToAdd;
	unorderedSet<char> modesToRemove;
	

	if (strModes[0] != '+' && strModes[0] != '-')
	{
		// ERROR PREFIX
		return (std::pair<unorderedSet<char>, unorderedSet<char> > ());
	}
	
    char currentSign = strModes[0];

    for (int i = 1; i < (int)strModes.size(); ++i)
    {
        char c = strModes[i];

        if (c == '+' || c == '-')
        {
            currentSign = c;
            continue ;
        }

        if (currentSign == '+')
            modesToAdd.insert(c);
        else
            modesToRemove.insert(c);
    }
		
    // Debug 
    std::cout << "Modes à ajouter : ";
    for (unorderedSet<char>::iterator it = modesToAdd.begin(); it != modesToAdd.end(); ++it)
	{
        std::cout << *it << " ";
	}
    std::cout << "\nModes à retirer : ";
    for (unorderedSet<char>::iterator it = modesToRemove.begin(); it != modesToRemove.end(); ++it)
	{
        std::cout << *it << " ";
	}
    std::cout << std::endl;
	
	return (std::pair<unorderedSet<char>, unorderedSet<char> >(modesToAdd, modesToRemove));
}
// first -> modeToAdd 
// second -> modeToRemove
void CommandHandler::cmdModesAddingSets(std::pair<unorderedSet<char>, unorderedSet<char> > modes, Channel *ch)
{
    for (unorderedSet<char>::iterator it = modes.first.begin(); it != modes.first.end(); ++it)
	{
        ch->addMode(*it);
	}
    for (unorderedSet<char>::iterator it = modes.second.begin(); it != modes.second.end(); ++it)
	{
        ch->removeMode(*it);
	}
}

char cmdModesWhichFirst(std::pair<std::vector<char>, std::vector<char> > modes)
{
	int l_index = 0;
	int o_index = 0;

    for (std::vector<char>::iterator it = modes.first.begin(); it != modes.first.end(); ++it)
	{
        if (*it == 'l')
		{
			break ;
		}
		l_index++;
	}
	for (std::vector<char>::iterator it = modes.first.begin(); it != modes.first.end(); ++it)
	{
        if (*it == 'o')
		{
			break ;
		}
		o_index++;
	}
    for (std::vector<char>::iterator it = modes.second.begin(); it != modes.second.end(); ++it)
	{

	}
}

//TODO: Execution + parsing des modes 'o' 'l'
void CommandHandler::cmdModes(int fd, std::istringstream &iss, Client &cl)
{
	std::string channelTarget;
	std::string strModes;
	std::string userTarget;
	int nbrUserRequired = 0;


	int channelId = parsingChannelId(iss, cl, channelTarget, "MODE");
    if (channelId < 0)
        return ;
    Channel *ch = _server->getChannelById(channelId);
	if (_server->isUserOperator(fd, channelId))
	{
		// ERROR USER USING COMMAND NOT OPERATOR OF THE CHANNEL
		return ;
	}
	
	iss >> strModes;
	if (cmdModesPreParsing(fd, strModes, cl, nbrUserRequired) == -1)
		return ;
	std::pair<unorderedSet<char>, unorderedSet<char> > modes(cmdModesParsingSets(fd, strModes, cl, ch));
	if (modes.first.empty() && modes.second.empty())
		return ;
	
	cmdModesAddingSets(modes, ch);
	// Parsing tageted user
	while (nbrUserRequired > 0)
	{
		iss >> userTarget;
		if (userTarget.empty())
		{
			// ERROR USER EMPTY
			return ;
		}
		int targetFd = _server->getClientFdByNickname(userTarget);
		if (targetFd < 0)
		{
			// ERROR NO SUCH USER
			return ;
		}
		if (_server->isUserAlreadyInChannel(targetFd, channelId) == false)
		{
			// ERROR TARGET USER NOT IN CHANNEL
			return ;
		}
		if (_server->isUserOperator(targetFd, channelId))
		{
			// EEROR ?? TARGET USER ALREADY OPERATOR
			return ;
		}
        nbrUserRequired--;
	}
	

}
