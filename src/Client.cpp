
#include "Client.hpp"

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
	}
	return (*this);
}

pollfd *Client::getPoll() const
{
	return (_pfd);
}

void Client::setPoll(pollfd *pfd)
{
	_pfd = pfd;
}