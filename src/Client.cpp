
#include "Client.hpp"

Client::Client() 
{

}

Client::Client(std::string username, std::string hostname, std::string servername, std::string realname, int clientFd) 
{
	_type = "client";
	_username = username;
	_hostname = hostname;
	_servername = servername;
	_realname = realname;
	_clientFd = clientFd;
}

Client::~Client() 
{

}

Client::Client(const Client &other)
{
	_type = other._type;
	_username = other._username;
	_hostname = other._hostname;
	_servername = other._servername;
	_realname = other._realname;
	_clientFd = other._clientFd;
}

Client &Client::operator=(const Client &other)
{
	if (this != &other)
	{
		_type = other._type;
		_username = other._username;
		_hostname = other._hostname;
		_servername = other._servername;
		_realname = other._realname;
		_clientFd = other._clientFd;
	}
	return (*this);
}

pollfd *Client::getPoll() const
{
	return (_pfd);
}

const std::string& Client::getUsername() const 
{ 
	return (_username);
}

const std::string& Client::getHostname() const 
{ 
	return (_hostname); 
}

const std::string& Client::getServername() const 
{ 
	return (_servername); 
}

const std::string& Client::getRealname() const 
{ 
	return (_realname); 
}

int Client::getFd() const 
{ 
	return (_clientFd);
}

void Client::setPoll(pollfd *pfd)
{
	_pfd = pfd;
}

void Client::sendMessage(const std::string &msg) const 
{
    if (_clientFd != -1) 
	{
		std::cout << "client fd (send msg): " << _clientFd << std::endl;
        std::string formatted = msg;
        if (formatted.find("\r\n") == std::string::npos)
            formatted += "\r\n";
        ssize_t res = send(_clientFd, formatted.c_str(), formatted.size(), 0);
        if (res == -1)
            herror("send");
    }
}