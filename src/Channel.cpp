#include "Channel.hpp"

Channel::Channel()
{

}

Channel::Channel(int id, std::string name, int newOperatorFd, int newUserFd)
{
    std::cout << "make object with 4 arguments" << std::endl;
    _id = id;
    _name = name;
    _operators.insert(newOperatorFd);
    _userSet.insert(newUserFd);
}

Channel::Channel(int id, std::string name, std::string password, int newOperatorFd, int newUserFd)
{
    std::cout << "make object with 5 arguments" << std::endl;
    _id = id;
    _name = name;
    _password = password;
    _operators.insert(newOperatorFd);
    _userSet.insert(newUserFd);
    if(!_password.empty())
        _modes.insert('k');
}

Channel::~Channel()
{

}

Channel::Channel(const Channel &other)
{
    _id = other._id;
    _name = other._name;
    _password = other._password;
    _operators = other._operators;
    _userSet = other._userSet;
}

Channel& Channel::operator=(const Channel &other)
{
    if (this != &other)
    {
        _id = other._id;
        _name = other._name;
        _password = other._password;
        _operators = other._operators;
        _userSet = other._userSet;
        _modes = other._modes;
    }
    return (*this);
}

const std::string Channel::getName() const
{
    return (_name);
}

const unordered_set<int>& Channel::getUserList() const 
{
    return _userSet;
}

const std::set<int>&Channel::getOperators() const
{
    return (_operators);
}

const std::string Channel::getPassword() const
{
    return (_password);
}

const int Channel::getId() const
{
	return (_id);
}

const std::set<int>& Channel::getModes() const
{
    return (_modes);
}

const std::string Channel::getTopic() const
{
    return (_topic);
}

void Channel::setTopic(std::string topic)
{
    _topic = topic;
}

void Channel::addMode(int mode)
{
    _modes.insert(mode);
}

void Channel::removeMode(int mode)
{
    _modes.erase(mode);
}

bool Channel::checkMode(int mode)
{
    return _modes.find(mode) != _modes.end();
}

bool Channel::checkInvitedUser(int invitedUser)
{
    return _invitedUser.find(invitedUser) != _invitedUser.end();
}

void Channel::inviteUser(int fd)
{
    _invitedUser.insert(fd);
}

void Channel::removeInvitedUser(int fd)
{
    _invitedUser.erase(fd);
}

bool Channel::isOperator(int fd) const
{
    return _operators.find(fd) != _operators.end();
}

void Channel::addOperator(int fd)
{
	_operators.insert(fd);
}

void Channel::removeOperator(int fd)
{
	_operators.erase(fd);
}

void Channel::addUser(int fd)
{
	_userSet.insert(fd);
}

void Channel::removeUser(int fd)
{
	_userSet.erase(fd);
}

void Channel::sendMessage(const std::string &msg) const
{
    for (unordered_set<int>::const_iterator itr = _userSet.begin(); itr != _userSet.end(); ++itr)
    {
        if (*itr != -1)
        {
            std::cout << "client fd (send msg): " << *itr << std::endl;

            std::string formatted = msg;
            if (formatted.find("\r\n") == std::string::npos)
                formatted += "\r\n";

            ssize_t res = send(*itr, formatted.c_str(), formatted.size(), 0);
            if (res == -1)
                herror("send");
        }
    }
}

void Channel::sendMessageAvoidingSender(const std::string &msg, int senderFd) const
{
    for (unordered_set<int>::const_iterator itr = _userSet.begin(); itr != _userSet.end(); ++itr)
    {
        if (*itr != -1 && *itr != senderFd)
        {
            std::cout << "client fd (send msg): " << *itr << std::endl;

            std::string formatted = msg;
            if (formatted.find("\r\n") == std::string::npos)
                formatted += "\r\n";

            ssize_t res = send(*itr, formatted.c_str(), formatted.size(), 0);
            if (res == -1)
                herror("send");
        }
    }
}
