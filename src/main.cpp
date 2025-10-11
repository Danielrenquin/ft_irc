#include "Server.hpp"
#include <stdlib.h>

int main(int argc, char **argv)
{
    if (argc != 2) 
    {
        std::cerr << "Usage: ./ircserv <port>\n";
        return 1;
    }

    try 
    {
        int port = std::atoi(argv[1]);
        Server server(port);
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
