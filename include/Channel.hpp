#pragma once

#include "Server.hpp"

class Channel
{
	private:
		int _id;
		std::string _name;
		std::string _password;
		std::string _topic;
		std::set<int> _operators;
		std::set<int> _invitedUser;
		std::set<int> _userSet;
		std::set<int> _modes;
	public:
		Channel();
		Channel(int id, std::string name, int newOperatorFd, int newUserFd);
		Channel(int id, std::string name, std::string password, int newOperatorFd, int newUserFd);
		~Channel();
		Channel(const Channel &other);
		Channel& operator=(const Channel &other);

		// Getters et setters
		const std::string getName() const;
		const std::string getPassword() const;
		const int getId() const;
		const std::set<int>&getUserList() const;
		const std::set<int>&getOperators() const;
		const std::set<int>&getModes() const;
		const std::string getTopic() const;
		void setTopic(std::string topic);
		
		bool isOperator(int fd) const;
		void addOperator(int fd);
		void removeOperator(int fd);
		void addUser(int fd);
		void removeUser(int fd);
		void addMode(int mode);
		void removeMode(int mode);

		void inviteUser(int fd);
		void removeInvitedUser(int fd);
		void sendMessage(const std::string &msg) const;
		void sendMessageAvoidingSender(const std::string &msg, int senderFd) const;
};


