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
/*
int Server::receiveDataFromUser(int clientFd, int clientIndex, char buffer[4][48], int *index)
{
	//const int BUFFER_SIZE = 1024;
    //char *buffer = new char[BUFFER_SIZE];
    
    //if (!buffer) 
		//return (NULL);
    
    int nbytes = 0;
    
    while (true)
    {
        nbytes = recv(clientFd, buffer[*index], sizeof(buffer[*index]), 0);
        //if (nbytes == -1)
			//throw std::runtime_error("recv failed");
        if (nbytes > 0) 
			break ;
		if (nbytes == 0)
		{
			std::cout << "Client " << clientFd << " disconnected" << std::endl;
			close(clientFd);
        	removeClient(clientIndex);
			return (-1);
		}
		
    }
    buffer[*index][nbytes] = '\0';
    return (0);
}

int Server::parsingUsername(int clientFd, int clientIndex, char buffer[4][48], int *index)
{
	write(clientFd, "write your username: ", 22);
	if (receiveDataFromUser(clientFd, clientIndex, buffer, index) == -1)
		return (-1);
	for (size_t i = 0; i < strlen(buffer[eUsername]); i++)
	{
		if (buffer[eUsername][i] == '\n')
			buffer[eUsername][i] = '\0';
	}
	if (strlen(buffer[eUsername]) == 0 || strlen(buffer[eUsername]) > 9)
	{
		write(clientFd, "Should not be empty and less than 10 char\n", 43);
		if (parsingUsername(clientFd, clientIndex, buffer, index))
			return (-1);
	}
	int i = 0;
	while (buffer[eUsername][i])
	{
		if (!isalnum(buffer[eUsername][i]) || buffer[eUsername][i] == ' ')
		{
			write(clientFd, "Contains unallowed char\n", 25);
			if (parsingUsername(clientFd, clientIndex, buffer, index))
				return (-1);
		}
		i++;
	}
	return (0);
}

int Server::parsingRealname(int clientFd, int clientIndex, char buffer[4][48], int *index)
{
	write(clientFd, "write your realname: ", 22);
	if (receiveDataFromUser(clientFd, clientIndex, buffer, index) == -1)
		return (-1);
	for (size_t i = 0; i < strlen(buffer[eUsername]); i++)
	{
		if (buffer[eUsername][i] == '\n')
			buffer[eUsername][i] = '\0';
	}
	if (strlen(buffer[eUsername]) == 0 || strlen(buffer[eUsername]) > 128)
	{
		write(clientFd, "Should not be empty and less than 128 char\n", 43);
		if (parsingUsername(clientFd, clientIndex, buffer, index))
			return (-1);
	}
}

int Server::parsingInputs(int clientFd, int clientIndex, char buffer[4][48])
{
	int index = 0;

	parsingUsername(clientFd, clientIndex, buffer, &index); // <-- index 0, username

	index++;
	write(clientFd, "write your hostname: ", 22);
	if (receiveDataFromUser(clientFd, clientIndex, buffer, &index) == -1) // <-- index 1, hostname, ignoré dans rfc1459 par le serveur
		return (-1);

	index++;
	write(clientFd, "write your servername: ", 24);
	if (receiveDataFromUser(clientFd, clientIndex, buffer, &index) == -1) // <-- index 2, servername, ignoré dans rfc1459 par le serveur
		return (-1);

	index++;
    parsingRealname(clientFd, clientIndex, buffer, &index); // <-- index 3, realname

	std::cout << buffer[eUsername] << std::endl;
	std::cout << buffer[eHostname] << std::endl;
	std::cout << buffer[eServername] << std::endl;
	std::cout << buffer[eRealname] << std::endl;
	return (0);
}

int Server::login(int clientIndex)
{
	static int mapId = 0;
	char buffer[4][48] = {}; // <-- utiliser l'index 0 à 3 pour construire un client
    struct sockaddr_storage clientAddr;
    socklen_t clientlen = sizeof(clientAddr);
    int clientFd = accept(_serverSocket, (struct sockaddr*)&clientAddr, &clientlen);
    if(clientFd == -1)
        return (-1);
    fcntl(clientFd, F_SETFL, O_NONBLOCK);
	
	if (parsingInputs(clientFd, clientIndex, buffer) == -1)
		return (-1);
	Client newClient(buffer[eUsername], 
					buffer[eHostname],
					buffer[eServername],
					buffer[eRealname],
					clientFd);
    //prend input et accept le client
    // construction du client --> conteneur de client
	_clients.insert(std::make_pair(mapId++, newClient));
	// Push pollfd dans le client
    addClient(clientFd); //ajout a un conteneur de client
    std::cout << "New client fd " << clientFd << std::endl;
	return (0);
}

void Server::handleClientData(size_t index)
{
    char buffer[MSG_SIZE];
    int fd = _pfds[index].fd;
    int nbytes = recv(fd, buffer, sizeof(buffer), 0);

    if(nbytes <= 0)
    {
        if(nbytes == 0)
            std::cout << "Client " << fd << " disconnected" << std::endl;
        else
            throw std::runtime_error("recv failed");
        close(fd);
        removeClient(index);
    }
    else
    {
		// Parsing commandes
		buffer[nbytes] = '\0';

		if (strncmp(buffer, "JOIN", 4) == 0)
		{
			std::cout << "[client: " << fd << "] want to join a channel" << std::endl;
		}
		// Implemter les autres parsing de commandes
		
		// Boucle afin d'envoyer le message aux autres clients
        for(size_t j = 1; j < _pfds.size(); ++j)
        {
            std::cout << "[recv FD " << fd << "]" << buffer << std::endl;
            int dest = _pfds[j].fd;
            // cette condition permet de pas envoyer le message 
            // a celui meme qui l'envoie
            if(dest != fd)
                send(dest, buffer, nbytes, 0);
        }
    }
}*/


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

/*
int Server::parsingUsername(std::string &msg, int fd, ClientContext &ctx)
{
	for (size_t i = 0; i < msg.size(); i++)
	{
		if (msg[i] == '\n')
			msg[i] = '\0';
	}
	if (msg.size() == 0 || msg.size()  > 9)
	{
		//write(fd, "Should not be empty and less than 10 char\n", 43);
		ctx.state = WAIT_USERNAME;
		return (-1);
	}
	int i = 0;
	while (msg[i])
	{
		if (!isalnum(msg[i]) || msg[i] == ' ')
		{
			write(fd, "Contains unallowed char\r\n", 26);
			ctx.state = WAIT_USERNAME;
			return (-1);
		}
		i++;
	}
	return (0);
}

int Server::parsingRealname(std::string &msg, int fd, ClientContext &ctx)
{
	for (size_t i = 0; i < msg.size(); i++)
	{
		if (msg[i] == '\n')
			msg[i] = '\0';
	}
	if (msg.size() == 0 || msg.size() > 128)
	{
		write(fd, "Should not be empty and less than 128 char\n", 43);
		ctx.state = WAIT_REALNAME;
		return (-1);
	}
	return (0);
}

void Server::contextSwitch(std::string &msg, ClientContext &ctx, Client &cl, int fd)
{
	switch (ctx.state) 
	{
		case WAIT_USERNAME:
			if (parsingUsername(msg, fd, ctx) == -1)
				return ;
			cl = Client(msg, "", "", "", fd);
			ctx.state = WAIT_HOSTNAME;
			cl.sendMessage("Enter hostname: ");
			break;
		case WAIT_HOSTNAME:
			cl = Client(cl.getUsername(), msg, "", "", fd);
			ctx.state = WAIT_SERVERNAME;
			cl.sendMessage("Enter servername: ");
			break;
		case WAIT_SERVERNAME:
			cl = Client(cl.getUsername(), cl.getHostname(), msg, "", fd);
			ctx.state = WAIT_REALNAME;
			cl.sendMessage("Enter realname: ");
			break;
		case WAIT_REALNAME:
			if (parsingRealname(msg, fd, ctx) == -1)
				return ;
			cl = Client(cl.getUsername(), cl.getHostname(), cl.getServername(), msg, fd);
			ctx.state = AUTHENTICATED;
			cl.sendMessage("Authenticated! You can now chat.\n");
			std::cout << "Client " << fd << " authenticated as " << cl.getUsername() << std::endl;
			break;
		case AUTHENTICATED:
			broadcastMessage(fd, cl.getUsername() + ": " + msg + "\n");
			break;
	}
}*/

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

void Server::cmdJoinAnExistingChannel(int fd, Client &cl, int targetId)
{
	Channel &ch = _channelList[targetId];

	if (!isUserAlreadyInChannel(fd, targetId)) 
		ch.addUser(fd);
	else 
		cl.sendMessage("NOTICE : You are already in this channel\r\n");

}

void Server::cmdJoin(int fd, std::istringstream &iss, Client &cl)
{
    static int id = 0;
    std::string userName = cl.getNickname();
    std::string channelName, pass;
	iss >> channelName;
	std::cout << "channel: " << channelName << std::endl;
	if (channelName.empty())
	{
        cl.sendMessage("ERROR :Channel name required\r\n");
		return ;
	}

	// Si le channel existe add le clientFd au set
	int targetId = getChannelIdByName(channelName);
    if (targetId != -1)
    {
		cmdJoinAnExistingChannel(fd, cl, targetId);
		Channel &ch = _channelList[targetId];
		// DEBUG
		std::cout << "existing channel: " << targetId << std::endl;
		for (std::set<int>::iterator it = ch.getUserList().begin(); it != ch.getUserList().end(); ++it)
		{
			std::cout << "user set: " << *it << std::endl;
		}
        return ;
    }  

	// Si le channel existe ne pas cree un nouvel objet
    iss >> pass;
	// Operateur ternaire afin de créer qu'une seule instances de channel
	Channel newChannel = pass.empty() ? Channel(id, channelName, fd, fd) : Channel(id, channelName, pass, fd, fd);   

	std::cout << id << std::endl;
    Channel &ch = newChannel;
    	_channelList[id++] = newChannel;
	ch.addOperator(fd);
	ch.addUser(fd);
	
	// DEBUG
	for (std::set<int>::iterator it = ch.getUserList().begin(); it != ch.getUserList().end(); ++it)
	{
		std::cout << "user set: " << *it << std::endl;
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
	return (0);
}

// TODO: implémenter l'envois de message dans les cannaux
void Server::cmdPrivMsg(int fd, std::istringstream &iss, Client &cl)
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
	
	int targetFd = getClientFdByNickname(targetUser);
	if (targetFd == 0)
	{
		cl.sendMessage("ERROR :Target User not found in the server\r\n");
		return ;
	}
	Client &clTarget = _clients[targetFd];

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
    else
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

    cl = Client(username, "", "", "", realname, fd); // A voir si les données ne seront pas écrasé lors de la construction
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

    cl = Client("", nick, "", "", "", fd); // A voir si les données ne seront pas écrasé lors de la construction
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

	/*if (isUserInAnyChannel(fd))
	{
		//TODO: implémenter une fonction qui enverra les msg du user dans ses channels
	}*/
	

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


