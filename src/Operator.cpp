#include "Operator.hpp"

Operator::Operator(std::string username, std::string hostname, std::string servername, std::string realname) : Client(username, hostname, servername, realname)
{
	_type = "operator";
}

Operator::~Operator()
{
}

Operator::Operator(const Operator &other) : Client(other)
{
	_type = "operator";
}

Operator &Operator::operator=(const Operator &other)
{
	if (this != &other)
	{
		_type = "operator";
		Client::operator=(other);
	}
	return (*this);
}
