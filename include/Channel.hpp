#pragma once

#include "Server.hpp"

class Channel
{
private:
	std::string _name;
	std::vector<std::string> _operators;
	//std::map<int, Client> _operators;
	std::vector<std::string> _userList;
public:
	Channel();
	~Channel();
	Channel(const Channel &other);
	Channel& operator=(const Channel &other);
};


