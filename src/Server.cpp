#include "Server.hpp"
#include "Channel.hpp"
#include "CommandHandler.hpp"


Server::Server(int port) : _port(port)
{
    _commandHandler = new CommandHandler(this);
    _commandHandler->registerCommands();
    if(port < 1 || port > 65535)
        throw std::invalid_argument("Port must be between 1 and 65535");
}

Server::~Server()
{
    if(_serverSocket != -1)
        close(_serverSocket);
    delete _commandHandler;
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

void Server::removeClient(int fd)
{
    for (std::vector<pollfd>::iterator it = _pfds.begin(); it != _pfds.end(); ++it)
    {
        if (it->fd == fd)
        {
            _pfds.erase(it);
            break;
        }
    }
    _clients.erase(fd);
    _contexts.erase(fd);
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

Channel *Server::getChannelById(int channelId)
{
	for (std::map<int, Channel>::iterator it = _channelList.begin(); it != _channelList.end(); it++)
	{
		Channel &ch = it->second; // Ici it->first correspond a l'id et it->second correspond au Client
		std::cout << "current channel name: " << ch.getName() << std::endl;
		if (ch.getId() == channelId)
		{
			std::cout << "ID FOUND" << std::endl;
			return (&ch);
		}
	}
	return (NULL);
}

Client *Server::getClientByFd(int fd)
{
	for (std::map<int, Client>::iterator it = _clients.begin(); it != _clients.end(); it++)
	{
		Client &cl = it->second; // Ici it->first correspond a l'id et it->second correspond au Client
		if (cl.getFd() == fd)
		{
			std::cout << "ID FOUND" << std::endl;
			return (&cl);
		}
	}
	return (NULL);
}

std::map<int, Channel>& Server::getChannelList()
{
    return _channelList;
}

std::map<int, Client>& Server::getClients()
{
    return _clients;
}

std::map<int, ClientContext>& Server::getContexts()
{
    return _contexts;
}


bool Server::isUserInAnyChannel(int fd)
{
	for (std::map<int, Channel>::iterator it = _channelList.begin(); it != _channelList.end(); ++it)
	{
		Channel &ch = it->second;
		const unordered_set<int> &users = ch.getUserList();

		if (users.find(fd) != users.end())
			return (true);
	}
	return (false);
}

bool Server::isUserOperator(int fd, int targetId)
{
 	Channel &ch = _channelList[targetId];
	for (std::set<int>::iterator it = ch.getOperators().begin(); it != ch.getOperators().end(); ++it)
	{
		if (*it == fd)
			return (true);
	}
	return (false);
}

bool Server::isAnExistingUser(int fd)
{
	for (std::map<int, Client>::iterator it = _clients.begin(); it != _clients.end(); ++it)
	{
		if (it->first == fd)
			return (true);
	}
	return (false);
}

bool Server::isUserAlreadyInChannel(int fd, int targetId)
{
	Channel &ch = _channelList[targetId];
	for (unordered_set<int>::const_iterator it = ch.getUserList().begin(); it != ch.getUserList().end(); ++it)
	{
		if (*it == fd)
			return (true);
	}
	return (false);
}

const int Server::getClientFdByNickname(std::string nickname)
{
	// Itérer une map
	for (std::map<int, Client>::iterator it = _clients.begin(); it != _clients.end(); it++)
	{
		Client &cl = it->second; // Ici it->first correspond a l'id et it->second correspond au Client
        std::cout << cl.getNickname() << "comparer avec" << nickname << std::endl;
		if (cl.getNickname() == nickname)
		{
			return (cl.getFd());
		}
	}
	return (-1);
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
        return;
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
    getline(iss, realname); // peut contenir espaces, commence souvent par ':'

    if (username.empty())
    {
        cl.sendMessage("461 * USER :Not enough parameters\r\n");
        return;
    }
    std::cout << "removefromchannels" << std::endl;
    if (!realname.empty() && realname[0] == ' ')
        realname = realname.substr(1);
    if (!realname.empty() && realname[0] == ':')
        realname = realname.substr(1);

    cl.setUsername(username);
    cl.setHostname(hostname);
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
        return;
    }
    std::cout << "dans la fonction cmdNick: " << nick << std::endl;
    cl.setNickname(nick);
    ctx.hasNick = true;

    if (ctx.hasPass && ctx.hasUser)
        registerClient(fd, ctx, cl);
}

//gère les deconnexions brutale du client
void Server::handleDisconected(int fd, int nbytes)
{
        Client &cl = _clients[fd];
        if(nbytes == 0)
        {
            std::cout << ":" + cl.getNickname() + "!" + cl.getUsername() + "@"
            + cl.getHostname() + "QUIT" + ":Connection reset by peer" + "\r\n";
        }
        close(fd);
        removeClient(fd);
        _clientBuffers.erase(fd);
}

//si la commande est complete le buffer de commande est copier
//dans la variable cmd et le buffer est libérer
bool Server::cmdIsComplete(int fd, std::string& cmd)
{
    size_t pos;
    size_t skip;

    pos = _clientBuffers[fd].find("\r\n");
    skip = 2;
    if(pos == std::string::npos)
    {
        pos = _clientBuffers[fd].find("\n");
        skip = 1;
    }
    if(pos == std::string::npos)
        return (0);
    cmd = _clientBuffers[fd].substr(0, pos);
    _clientBuffers[fd].erase(0, pos + skip);
    return(1);
}

void Server::handleClientData()
{
    int fd = _pfds[_pfdsIndex].fd;
    char buffer[MSG_SIZE];
    int nbytes = recv(fd, buffer, sizeof(buffer) - 1, 0);
    std::string cmd;

    // Déconnexion du client
    if (nbytes <= 0)
    {
        handleDisconected(fd, nbytes);
        return;
    }

    // Ajouter les données reçues au buffer existant
    buffer[nbytes] = '\0';
    _clientBuffers[fd] += std::string(buffer, nbytes);

    // Boucle tant qu'il y a une commande complète (\n ou \r\n)
    while (true)
    {
        //vois si la fonction est complète ou pas et la traite partielement si complète
        if(!cmdIsComplete(fd, cmd))
            break;
        cmd = trim(cmd);
        if (cmd.empty())
            continue;

        std::istringstream iss(cmd);
        std::string command;
        iss >> command;
        Client &cl = _clients[fd];
        ClientContext &ctx = _contexts[fd];

        // Gestion de l'authentification ou execution de commande 
        if (command == "PASS")
            cmdPass(fd, iss, ctx, cl);
        else if (command == "NICK")
            cmdNick(fd, iss, ctx, cl);
        else if (command == "USER")
            cmdUser(fd, iss, ctx, cl);
        else if (ctx.state != AUTHENTICATED)
            cl.sendMessage("ERROR :You must register first\r\n");
        else
            _commandHandler->executeCommand(cmd, fd, cl);
    }
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