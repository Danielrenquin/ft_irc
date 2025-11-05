#include "Server.hpp"
#include "Channel.hpp"

class Channel;

// Structure de context d'authentification
// Permet de savoir l'état actuel d'authentification du client
struct ClientContext 
{
    Server::ClientState state;
    bool hasPass;
    bool hasNick;
    bool hasUser;
};

Server::Server(int port) : _port(port)
{
    if(port < 1 || port > 65535)
        throw std::invalid_argument("Port must be between 1 and 65535");
}

Server::~Server()
{
    if(_serverSocket != -1)
        close(_serverSocket);
}

void Server::setPassword(char *password)
{
    _password = password;
}

void Server::initSocket(char **argv)
{
    struct addrinfo hints, *res, *p;
    int status;
    int yes = 1;
    const char *portStr = argv[1];

    //initalisation de hints afin de donner les seting a getaddrinfo
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    //getaddrinfo créé une liste chainée de structures addrinfo
    //res est un pointeur sur la première structure de cette liste
    if((status = getaddrinfo(NULL, portStr, &hints, &res)) != 0)
        throw std::runtime_error(std::string("getaddrinfo: ") + gai_strerror(status));

    //itération  sur la liste chainée
    for(p = res; p != NULL; p = p->ai_next)
    {
        _serverSocket = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if(_serverSocket == -1)
            continue;

        //redemarre le serveur sans interuption en cas de problème
        setsockopt(_serverSocket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

        //set le serveur en non bloquant
        fcntl(_serverSocket, F_SETFL, O_NONBLOCK);

        //lie le socket a la strucuture avec les bonne data
        if (bind(_serverSocket, p->ai_addr, p->ai_addrlen) == 0)
        {
            std::cout << "Bound successfully on port " << _port << std::endl;
            break;
        }
        close(_serverSocket);
        _serverSocket = -1;
    }
    freeaddrinfo(res);
    if(_serverSocket == -1)
        throw std::runtime_error("Failled to bind socket on any address");
}

void Server::startListening()
{
    if(listen(_serverSocket, PENDING_QUEUE_MAXLENGTH))
        throw std::runtime_error("listen failled");
    std::cout << "Server listening on port " << _port << std::endl;

    pollfd serverPfd;
    serverPfd.fd = _serverSocket;
    serverPfd.events = POLLIN;
    serverPfd.revents = 0;
    _pfds.push_back(serverPfd);

    handleConnections();
}

void Server::addClient(int fd)
{
    pollfd pfd;
    pfd.fd = fd;
    pfd.events = POLLIN; //alert quand je peux envoyer les datas au socket
    pfd.revents = 0;
    _pfds.push_back(pfd);
}

void Server::removeClient()
{
    std::swap(_pfds[_pfdsIndex], _pfds.back());
    _pfds.pop_back();
}

void Server::printClients() const
{
    for (std::map<int, Client>::const_iterator it = _clients.begin(); it != _clients.end(); ++it)
    {
        std::cout << "FD: " << it->first 
				  << " | Nickname: " << it->second.getNickname()
				  << " | Username: " << it->second.getUsername()
				  << " | Hostname: " << it->second.getHostname()
				  << " | Servername: " << it->second.getServername()
				  << " | Realname: " << it->second.getRealname()
                  << std::endl;
    }
}

// Permet de parser le buffer
std::string Server::trim(const std::string &s) 
{
    size_t start = s.find_first_not_of(" \n\r\t");
    size_t end = s.find_last_not_of(" \n\r\t");

    if (start == std::string::npos) 
		return ("");

    return (s.substr(start, end - start + 1));
}

// Envois les messages sur les autres fd
void Server::broadcastMessage(int senderFd, const std::string &msg) 
{
    for (size_t i = 1; i < _pfds.size(); ++i) 
	{
        int fd = _pfds[i].fd;

        if (fd != senderFd)
            _clients[fd].sendMessage(msg);
    }
}

// Authentification du nouveau client
void Server::acceptNewClient()
{
    struct sockaddr_storage clientAddr;
    socklen_t clientlen = sizeof(clientAddr);
    int clientFd = accept(_serverSocket, (struct sockaddr*)&clientAddr, &clientlen);

    if (clientFd == -1)
        return;

    // Mettre en mode non-bloquant
    fcntl(clientFd, F_SETFL, O_NONBLOCK);

	Client newClient("", "", "", "", "", clientFd);
    _clients[clientFd] = newClient;

    // Context client
    ClientContext ctx;
    ctx.state = WAIT_PASS;
    ctx.hasPass = false;
    ctx.hasNick = false;
    ctx.hasUser = false;
    _contexts[clientFd] = ctx;

    // Ajouter le client au poll
    addClient(clientFd);

    std::cout << "New client connected [fd=" << clientFd << "]" << std::endl;
}

const int Server::getChannelIdByName(std::string &channelName)
{
	for (std::map<int, Channel>::iterator it = _channelList.begin(); it != _channelList.end(); it++)
	{
		Channel &ch = it->second; // Ici it->first correspond a l'id et it->second correspond au Client
		std::cout << "current channel name: " << ch.getName() << std::endl;
		if (ch.getName() == channelName)
		{
			std::cout << "ID FOUND" << std::endl;
			return (ch.getId());
		}
	}
	return (-1);
}

const int Server::getChannelIdByClientFd(int fd)
{
	for (std::map<int, Channel>::iterator it = _channelList.begin(); it != _channelList.end(); it++)
	{
		Channel &ch = it->second; // Ici it->first correspond a l'id et it->second correspond au Client
		const std::set<int> &users = ch.getUserList();

		if (users.find(fd) != users.end())
			return (ch.getId());
	}
	return (-1);	
}

bool Server::isUserInAnyChannel(int fd)
{
	for (std::map<int, Channel>::iterator it = _channelList.begin(); it != _channelList.end(); ++it)
	{
		Channel &ch = it->second;
		const std::set<int> &users = ch.getUserList();

		if (users.find(fd) != users.end())
			return (true);
	}
	return (false);
}

bool Server::isUserAlreadyInChannel(int fd, int targetId)
{
	Channel &ch = _channelList[targetId];
	for (std::set<int>::iterator it = ch.getUserList().begin(); it != ch.getUserList().end(); ++it)
	{
		if (*it == fd)
			return (true);
	}
	return (false);
}

int Server::cmdJoinAnExistingChannel(int fd, Client &cl, int targetId, std::string password)
{
	Channel &ch = _channelList[targetId];
	std::string formattedMsg = ":" + cl.getNickname() + "!" + cl.getUsername() + "@" + cl.getHostname() + " JOIN " + " :" + ch.getName();

	if (!isUserAlreadyInChannel(fd, targetId))
	{
		if (!ch.getPassword().empty())
		{
			if (password == ch.getPassword())
			{
				ch.sendMessage(formattedMsg);
				ch.addUser(fd);
				return (0);
			}
			else
			{
				cl.sendMessage("ERROR : Incorrect password\r\n");
				return (-1);
			}
		}
		ch.sendMessage(formattedMsg);
		ch.addUser(fd);
	}
	else
	{
		cl.sendMessage("NOTICE : You are already in this channel\r\n");
	}
	return (0);
}

std::pair<std::vector<std::string>, std::vector<std::string>> Server::fillListCmdJoin(std::istringstream &iss)
{
	std::string channelsToken;
	std::string keysToken;

	iss >> channelsToken;
	iss >> keysToken;

	// split channels
	std::vector<std::string> channelList;
	std::stringstream cstream(channelsToken);
	std::string chan;
	while (std::getline(cstream, chan, ','))
		channelList.push_back(chan);

	// split keys
	std::vector<std::string> passList;
	if (!keysToken.empty()) 
	{
		std::stringstream kstream(keysToken);
		std::string key;
		while (std::getline(kstream, key, ','))
			passList.push_back(key);
	}
	return {channelList, passList};
}

void Server::cmdJoin(int fd, std::istringstream &iss, Client &cl)
{
    static int id = 0;
	std::pair<std::vector<std::string>, std::vector<std::string>> data = fillListCmdJoin(iss);
	std::vector<std::string> channelList = data.first;
	std::vector<std::string> passList = data.second;

    for (size_t i = 0; i < channelList.size(); i++)
    {
        std::string &channel = channelList[i];
        std::string pass = (i < passList.size()) ? passList[i] : "";

        // Check prefix
        if (channel.empty() ||
            (channel[0] != '#' && channel[0] != '&' &&
             channel[0] != '!' && channel[0] != '+'))
        {
            cl.sendMessage("ERROR :Need a prefix\r\n");
            continue ;
        }

        // Check channel exists
        int targetId = getChannelIdByName(channel);
        if (targetId != -1)
        {
			std::string password = passList.empty() ? "" : passList[i];
			if (cmdJoinAnExistingChannel(fd, cl, targetId, password) == -1)
			{
				return ;
			}
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
		// DEBUG
		/*for (std::set<int>::iterator it = ch.getUserList().begin(); it != ch.getUserList().end(); ++it)
		{
			std::cout << "user set: " << *it << std::endl;
		}*/
    }
}

const int Server::getClientFdByNickname(std::string nickname)
{
	// Itérer une map
	for (std::map<int, Client>::iterator it = _clients.begin(); it != _clients.end(); it++)
	{
		Client &cl = it->second; // Ici it->first correspond a l'id et it->second correspond au Client
		if (cl.getNickname() == nickname)
		{
			return (cl.getFd());
		}
	}
	return (-1);
}

std::vector<std::string> Server::parsePrivMsg(std::istringstream &iss, Client &cl, std::string &userMsg)
{
	std::vector<std::string> targets;
	std::string target;
	int i = 0;

	while (std::getline(iss, target, ','))
	{
		// parse le dernier target que getline() trouve qui est le message du user
		size_t pos = target.find(":");
		if (pos != std::string::npos)
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

		if (i > ERR_TOOMANYTARGETS)
		{
			cl.sendMessage("ERROR :Too many targets\r\n");
			// Return un vecteur vide en cas d'erreur
			return {};
		}
		i++;
	}
	return (targets);
}

void Server::sendPrivMsg(Client &cl, std::vector<std::string> &targets, std::vector<WhichTarget> &whichTarget, std::string &userMsg)
{
	// String formaté a la norme RFC
	std::string formattedMsg = ":" + cl.getNickname() + "!" + cl.getUsername() + "@" + cl.getHostname() + " PRIVMSG ";

    for (size_t i = 0; i < whichTarget.size(); i++)
    {
		WhichTarget wt = whichTarget[i];
        int targetFd = wt.id;

        std::string fullMsg = formattedMsg + targets[i] + " :" + userMsg + "\r\n";
		
		if (wt.type == WhichTarget::CHANNEL) 
		{
			Channel &targetCh = _channelList[targetFd];
			targetCh.sendMessageAvoidingSender(fullMsg, cl.getFd());
		}
		else 
		{
			Client &targetCl = _clients[targetFd];
			targetCl.sendMessage(fullMsg);
		}
    }
}

std::vector<Server::WhichTarget> Server::validatePrivMsg(Client &cl, std::vector<std::string> &targets)
{
	std::vector<WhichTarget> whichTarget;

	for (size_t i = 0; i < targets.size(); i++) 
	{
		if (targets[i].empty()) 
		{
			cl.sendMessage("ERROR :Target User required\r\n");
			return {};
		}

		WhichTarget wt;
		size_t pos = targets[i].find("#");
		if (pos != std::string::npos) 
		{
			wt.type = WhichTarget::CHANNEL;
			wt.id = getChannelIdByName(targets[i]);
			if (wt.id < 0) 
			{
				cl.sendMessage("ERROR " + targets[i] + " :No such channel\r\n");
				return {};
			}
		} 
		else 
		{
			wt.type = WhichTarget::USER;
			wt.id = getClientFdByNickname(targets[i]);
			if (wt.id < 0) 
			{
				cl.sendMessage("401 " + targets[i] + " :No such nick\r\n");
				return {};
			}
		}
		whichTarget.push_back(wt);
	}
	return (whichTarget);
}

// vector de struct car id identique non possible dans map<> seulement un id client peut etre identique a un id channel
void Server::cmdPrivMsg(int fd, std::istringstream &iss, Client &cl)
{
    std::vector<std::string> targets;
	std::vector<int> targetsFds;
	std::vector<WhichTarget> whichTarget;
	std::string userMsg;
    std::string target;

	// Parsing des target users
	targets = parsePrivMsg(iss, cl, userMsg);
	if (targets.empty())
		return ;

	// DEBUG
	for (size_t i = 0; i < targets.size(); i++) 
    {
        std::cout << "'" + targets[i] + "'" << "\n";
    }
	printClients();

    // Validité des users
	whichTarget = validatePrivMsg(cl, targets);
	if (whichTarget.empty())
		return ;

	// Send msg pour chaque user
	sendPrivMsg(cl, targets, whichTarget, userMsg);
}

// TODO: retirer les clients lors de leurs deconnexion ainsi que les channel
void Server::cmdQuit(int fd, Client &cl)
{
    std::cout << "Client " << fd << " disconnected" << std::endl;
    cl.sendMessage("[Server]: Goodbye, " + cl.getUsername() + "\r\n");
    close(fd);
    removeClient();
    _clients.erase(fd);
    _contexts.erase(fd);
}

void Server::isCmd(std::string &msg, int fd, std::istringstream &iss, Client &cl)
{
    // Convertir la commande en majuscules pour éviter les erreurs
    std::string command = msg.substr(0, msg.find(' '));
	// ransform() Applique a chaque charactère itérée une fonction et retourn le résultat dans son 3eme argument
	// Encore une fonction cheaté du c++ :/
    std::transform(command.begin(), command.end(), command.begin(), ::toupper);

    if (command == "JOIN")
    {
        std::cout << "[client: " << fd << "] wants to JOIN a channel" << std::endl;
        cmdJoin(fd, iss, cl);
    }
    else if (command == "PRIVMSG")
    {
        cmdPrivMsg(fd, iss, cl);
    }
    else if (command == "QUIT")
    {
        cmdQuit(fd, cl);
    }
    else if (!isUserInAnyChannel(fd))
    {
        cl.sendMessage("Unknown command: " + command + "\r\n");
    }
}

void Server::registerClient(int fd, ClientContext &ctx, Client &cl)
{
    ctx.state = AUTHENTICATED;

    std::ostringstream ss;
    ss << ":server " << cl.getNickname() << " :Welcome to the IRC Network " << cl.getNickname() << "!" << cl.getUsername() << "@localhost\r\n";
    cl.sendMessage(ss.str());

    std::cout << "Client " << fd << " registered as " << cl.getNickname() << " (" << cl.getUsername() << ")" << std::endl;
}


void Server::cmdPass(int fd, std::istringstream &iss, ClientContext &ctx, Client &cl)
{
    std::string pass;
    iss >> pass;

    if (pass.empty()) 
    {
        cl.sendMessage("ERROR :Password required\r\n");
        return ;
    }

    if (pass != _password) 
    { 
        // _password défini au lancement du serveur
        cl.sendMessage("ERROR :Invalid password\r\n");
        close(fd);
        return ;
    }

    ctx.hasPass = true;
    cl.sendMessage(":server NOTICE * :Password accepted\r\n");

    if (ctx.hasNick && ctx.hasUser)
        registerClient(fd, ctx, cl);
}

void Server::cmdUser(int fd, std::istringstream &iss, ClientContext &ctx, Client &cl)
{
    std::string username, hostname, servername, realname;
    iss >> username >> hostname >> servername;
    getline(iss, realname);

    if (username.empty()) 
	{
        cl.sendMessage("461 * USER :Not enough parameters\r\n");
        return ;
    }

    if (username.find_first_of(",#&:") != std::string::npos ||
        hostname.find_first_of(",#&:") != std::string::npos ||
        servername.find_first_of(",#&:") != std::string::npos ||
        realname.find_first_of(",#&:") != std::string::npos) 
    {
        cl.sendMessage("USER :Unauthorized characters\r\n");
        return ;
    }

    cl.setUsername(username);
    cl.setHostname(hostname);
    cl.setServername(servername);
    cl.setRealname(realname);

    ctx.hasUser = true;

    if (ctx.hasPass && ctx.hasNick)
        registerClient(fd, ctx, cl);
}


void Server::cmdNick(int fd, std::istringstream &iss, ClientContext &ctx, Client &cl)
{
    std::string nick;
    iss >> nick;

    if (nick.empty())
	{
        cl.sendMessage("431 * :No nickname given\r\n");
        return ;
    }
    if (nick.find_first_of(" ,#&:") != std::string::npos)
	{
        cl.sendMessage("NICK :Unauthorized characters\r\n");
        return ;
    }

    cl.setNickname(nick);
    ctx.hasNick = true;

    if (ctx.hasPass && ctx.hasUser)
        registerClient(fd, ctx, cl);
}

void Server::handleClientData()
{
    int fd = _pfds[_pfdsIndex].fd;
    char buffer[MSG_SIZE];
    int nbytes = recv(fd, buffer, sizeof(buffer) - 1, 0);

	// Deconnexion et reset du client
    if (nbytes <= 0)
    {
        if (nbytes == 0)
            std::cout << "Client " << fd << " disconnected" << std::endl;

        close(fd);
        removeClient();
        _clients.erase(fd);
        _contexts.erase(fd);
        return ;
    }

    buffer[nbytes] = '\0';
    std::string msg = trim(buffer);

	// Setup du client et de son context
    Client &cl = _clients[fd];
    ClientContext &ctx = _contexts[fd];

	std::cout << "client fd (clientdata); " << cl.getFd() << std::endl;

    // Authentification du client
    std::istringstream iss(msg);
    std::string command;
    iss >> command;
	
	if (isUserInAnyChannel(fd))
	{
		int channelId = getChannelIdByClientFd(fd);
		Channel &ch = _channelList[channelId];
		std::string formattedString = ":" + cl.getNickname() + "!" + cl.getUsername() + "@" + cl.getHostname() + " PRIVMSG " + ch.getName() + " :" + msg + "\r\n";
		//TODO: DEBUGING
		ch.sendMessageAvoidingSender(formattedString, fd);
	}
    if (command == "PASS")
        cmdPass(fd, iss, ctx, cl);
    else if (command == "NICK")
        cmdNick(fd, iss, ctx, cl);
    else if (command == "USER")
        cmdUser(fd, iss, ctx, cl);
    else if (ctx.state != AUTHENTICATED)
       cl.sendMessage("ERROR :You must register first\r\n");
	else
		isCmd(msg, fd, iss, cl);

}

void Server::handleConnections()
{
    while (true)
    {
        //poll renvoie le nombre de fd qui ont déclencher un évenement
        int pollResult = poll(_pfds.data(), _pfds.size(), -1);
        if(pollResult < 0)
            throw std::runtime_error("poll failled");

        for(_pfdsIndex = 0; _pfdsIndex < _pfds.size(); _pfdsIndex++)
        {
            //un nouveaux client se logue sur le server
            //Si c' est le socket sever et que revents a pris la valeur du flag POLLIN
            if(_pfds[_pfdsIndex].fd == _serverSocket && (_pfds[_pfdsIndex].revents & POLLIN))
            {
				acceptNewClient();
            }
            //si un client a envoyer des données
            //ou si un client se deconnect du server
            else if (_pfds[_pfdsIndex].revents &POLLIN)
            {
                handleClientData();
            }
        }
    }
}


