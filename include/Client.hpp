#pragma once

#include "Server.hpp"

class Server;

class Client
{
	private:
		std::string _type;
		int _clientFd;
		pollfd *_pfd;
    protected:
        std::string _username;
		std::string _nickname;
        std::string _hostname;
		std::string _servername;
        std::string _realname;
    public:
	 	Client();
        Client(std::string username, std::string nickname, std::string hostname, std::string servername, std::string realname, int clientFd);
        ~Client();
		Client(const Client &other);
		Client& operator=(const Client &other);

		pollfd *getPoll() const;
		void setPoll(pollfd *pfd);
		const std::string& getUsername() const;
		const std::string& getHostname() const;
		const std::string& getServername() const;
		const std::string& getRealname() const;
		const std::string& getNickname() const;
		void setUsername(const std::string &username);
		void setNickname(const std::string &nickname);
		void setHostname(const std::string &hostname);
		void setRealname(const std::string &realname);
		int getFd() const;

		void sendMessage(const std::string &msg) const;

};
