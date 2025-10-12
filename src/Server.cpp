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

/*void Server::startListening()
{
    //met le socket en mode écoute
    if(listen(_serverSocket, PENDING_QUEUE_MAXLENGTH))
        throw std::runtime_error("listen failed");

    while (true)
    {
        //cette structure permet de contenir une structure adapter à ipv6 ou ipv4
        //celle ci sera caster par l' os au moment du call de la fonction accept
        //en fonction du type de connexion(ipv6 ou ipv4)
        struct sockaddr_storage clientAddr;
        socklen_t clientLen = sizeof(clientAddr);

        int clientSocket = accept(_serverSocket, (struct sockaddr *)&clientAddr, &clientLen);
        if (clientSocket == -1)
            continue;
        
        //--------------seulement pour affichage a delete plus tard---------------
        char host[NI_MAXHOST];
        char service[NI_MAXSERV];

        int result = getnameinfo(
            (struct sockaddr *)&clientAddr, clientLen,
            host, sizeof(host),
            service, sizeof(service),
            NI_NUMERICHOST | NI_NUMERICSERV);
        

        if (result != 0)
        {
            std::cerr << "getnameinfo: " << gai_strerror(result) << std::endl;
            close(clientSocket);
            continue;
        }

        std::cout << "New client connected!" << std::endl;
        std::cout << "  - FD: " << clientSocket << std::endl;
        std::cout << "  - IP: " << host << std::endl;
        std::cout << "  - Port: " << service << std::endl;
        std::cout << "--------------------------------" << std::endl;
        //--------------------------------------------------------------------------
    }
}*/

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

    handel_connecions();
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

void Server::handel_connecions()
{
    while (true)
    {
        //poll renvoie le nombre de fd qui ont déclencher un évenement
        int pollResult = poll(_pfds.data(), _pfds.size(), -1);
        if(pollResult < 0)
            throw std::runtime_error("poll failled");
        
        for(size_t i = 0; i < _pfds.size(); i++)
        {
            //Si c' est le socket sever et que revents a pris la valeur du flag POLLIN
            if(_pfds[i].fd == _serverSocket && (_pfds[i].revents & POLLIN))
            {
                struct sockaddr_storage clientAddr;
                socklen_t clientlen = sizeof(clientAddr);
                int clientFd = accept(_serverSocket, (struct sockaddr*)&clientAddr, &clientlen);
                if(clientFd == -1)
                    continue;
                fcntl(clientFd, F_SETFL, O_NONBLOCK);
                addClient(clientFd);
                std::cout << "New client fd " << clientFd << std::endl;
            }
            //si un client a envoyer des données
            else if (_pfds[i].revents &POLLIN)
            {
                char buffer[MSG_SIZE];
                int clientFd = _pfds[i].fd;
                int nbytes = recv(clientFd, buffer, sizeof(buffer), 0);

                //deconnexion du client ou erreur
                if(nbytes <= 0)
                {
                    if(nbytes == 0)
                        std::cout << "Client FD " << clientFd << "disconnected" << std::endl;
                    else
                        throw std::runtime_error("recv failled");
                    close(clientFd);
                    removeClient(i);
                    --i;
                }
                //broadcast du message
                else
                {
                    buffer[nbytes] = '\0';
                    std::cout << "[recv FD " << clientFd << "]" << buffer << std::endl;
                    for(size_t j = 0; j < _pfds.size(); j++)
                    {
                        if(_pfds[j].fd != _serverSocket && _pfds[j].fd != clientFd)
                            send(_pfds[j].fd, buffer, nbytes, 0);
                    }
                }
            }
        }
    }
}