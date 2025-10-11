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

void::Server::initSocket(char **argv)
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
        {
            continue;
        }
        
        /*--------------seulement pour affichage a delete plus tard---------------*/
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
        /*--------------------------------------------------------------------------*/
    }
}
