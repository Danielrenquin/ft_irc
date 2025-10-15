#pragma once

#include "Client.hpp"

class Operator : public Client
{
private:
	std::string _type;
public:
	Operator(std::string username, std::string hostname, std::string servername, std::string realname);
	~Operator();
	Operator(const Operator &other);
	Operator &operator=(const Operator &other);
};
