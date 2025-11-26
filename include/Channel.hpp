#pragma once

#include "Server.hpp"
#include "unorderedset.hpp"

class Channel
{
	private:
		int _id;
		std::string _name;
		std::string _password;
		std::string _topic;
		std::set<int> _operators;
		std::set<int> _invitedUser;
		unordered_set<int> _userSet;
		std::set<int> _modes;
		bool _hasLimit;
    	int  _limit;
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
		int getId() const;
		const unordered_set<int>& getUserList() const;
		const std::set<int>&getOperators() const;
		const std::set<int>&getModes() const;
		const std::string getTopic() const;
		void setKey(const std::string& k);
		void removeKey();
		void setTopic(std::string topic);

		bool isOperator(int fd) const;
		bool isModeAlreadyInSet(int mode) const;
		void addOperator(int fd);
		void removeOperator(int fd);
		void addUser(int fd);
		void removeUser(int fd);
		void addMode(int mode);
		void removeMode(int mode);
		void setLimit(int limit);
		void removeLimit();
		bool hasMode(char mode) const;
		bool hasLimit() const;
		int  getLimit() const;
		int  getUserCount() const;
		bool checkMode(int mode);
		bool checkInvitedUser(int invitedUser);

		void inviteUser(int fd);
		void removeInvitedUser(int fd);
		void sendMessage(const std::string &msg) const;
		void sendMessageAvoidingSender(const std::string &msg, int senderFd) const;
};
