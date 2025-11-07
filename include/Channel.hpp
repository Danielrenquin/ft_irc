#pragma once

#include "Server.hpp"

//retirer l' utilisateur des channel lorsqu'il se deconnect

class Channel
{
	private:
		int _id;
		std::string _name;
		std::string _password;
		std::set<int> _operators;
		std::set<int> _invitedUser;
		std::set<int> _userSet;
	public:
		Channel();
		Channel(int id, std::string name, int newOperatorFd, int newUserFd);
		Channel(int id, std::string name, std::string password, int newOperatorFd, int newUserFd);
		~Channel();
		Channel(const Channel &other);
		Channel& operator=(const Channel &other);

		// Getters et setters
		const std::string getName() const;
		const int getId() const;
		const std::set<int>&getUserList() const;
		const std::set<int>&getOperators() const;
		bool isOperator(int fd) const;

		void addOperator(int fd);
		void removeOperator(int fd);
		void addUser(int fd);
		void removeUser(int fd);
		void inviteUser(int fd);
		void removeInvitedUser(int fd);
};
