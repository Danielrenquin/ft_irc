#pragma once

#include "Server.hpp"

class Channel
{
private:
	int _id;
	std::string _name;
	std::string _password;
	std::vector<std::string> _operators;
	//std::map<int, Client> _operators;
	std::vector<std::string> _userList;
public:
	Channel(int id, std::string name, std::string newOperator, std::string newUser);
	Channel(int id, std::string name, std::string password, std::string newOperator, std::string newUser);
	~Channel();
	Channel(const Channel &other);
	Channel& operator=(const Channel &other);

	// Getters et setters
	const std::string getName() const;
	const std::vector<std::string> getUserList() const;
	const std::vector<std::string> getOperators() const;
};


