#include "Server.hpp"


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

    handle_connections();
}

void Server::addClient(int fd)
{
    pollfd pfd;
    pfd.fd = fd;
    pfd.events = POLLIN; //alert quand je peux envoyer les datas au socket
    pfd.revents = 0;
    _pfds.push_back(pfd);
}

void Server::removeClient(int index)
{
    std::swap(_pfds[index], _pfds.back());
    _pfds.pop_back();
}

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
	(*index)++;
    return (0);
}

int Server::parsingInputs(int ClientFd, int clientIndex, char buffer[4][48])
{
	int index = 0;

	write(ClientFd, "write your username: ", 22);
	if (receiveDataFromUser(ClientFd, clientIndex, buffer, &index) == -1) // <-- index 0, username
		return (-1);

	write(ClientFd, "write your hostname: ", 22);
	if (receiveDataFromUser(ClientFd, clientIndex, buffer, &index) == -1) // <-- index 1, hostname
		return (-1);

	write(ClientFd, "write your servername: ", 24);
	if (receiveDataFromUser(ClientFd, clientIndex, buffer, &index) == -1) // <-- index 2, servername
		return (-1);

	write(ClientFd, "write your realname: ", 22);
	if (receiveDataFromUser(ClientFd, clientIndex, buffer, &index) == -1) // <-- index 3, realname
		return (-1);

	std::cout << buffer[0] << std::endl;
	std::cout << buffer[1] << std::endl;
	std::cout << buffer[2] << std::endl;
	std::cout << buffer[3] << std::endl;
	return (0);
}

int Server::login(int clientIndex)
{
	char buffer[4][48] = {0}; // <-- utiliser l'index 0 à 3 pour construire un client
    struct sockaddr_storage clientAddr;
    socklen_t clientlen = sizeof(clientAddr);
    int clientFd = accept(_serverSocket, (struct sockaddr*)&clientAddr, &clientlen);
    if(clientFd == -1)
        return (-1);
    fcntl(clientFd, F_SETFL, O_NONBLOCK);
	
	if (parsingInputs(clientFd, clientIndex, buffer) == -1)
		return (-1);
	
	// scanf lire sur le fd du client 
    //prend input et accept le client
    // construction du client --> conteneur de client
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
        for(size_t j = 1; j < _pfds.size(); ++j)
        {
            buffer[nbytes] = '\0';
            std::cout << "[recv FD " << fd << "]" << buffer << std::endl;
            int dest = _pfds[j].fd;
            //cette condition permet de pas envoyer le message 
            //a celui meme qui l' envoie
            if(dest != fd)
                send(dest, buffer, nbytes, 0);
        }
    }
}

void Server::handle_connections()
{
    while (true)
    {
        //poll renvoie le nombre de fd qui ont déclencher un évenement
        int pollResult = poll(_pfds.data(), _pfds.size(), -1);
        if(pollResult < 0)
            throw std::runtime_error("poll failled");

        for(size_t i = 0; i < _pfds.size(); i++)
        {
            //un nouveaux client se logue sur le server
            //Si c' est le socket sever et que revents a pris la valeur du flag POLLIN
            if(_pfds[i].fd == _serverSocket && (_pfds[i].revents & POLLIN))
            {
				// parsing
				// Client first("user", "...");
				if (login(i) == -1)
					break ;
            }
            //si un client a envoyer des données
            //ou si un client se deconnect du server
            else if (_pfds[i].revents &POLLIN)
            {
                handleClientData(i);
            }
        }
    }
}
