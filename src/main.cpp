#include "Server.hpp"
#include <stdlib.h>

//implémenter input user et ces 4 parametre

int main(int argc, char **argv)
{
    if (argc != 3)
    {
        std::cerr << "Usage: ./ircserv <port> <pass>\n";
        return 1;
    }

    try
    {
        int port = std::atoi(argv[1]);

        Server server(port);
        server.setPassword(argv[2]);
        server.initSocket(argv);
        server.startListening();
    }

    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
