#include "Channel.hpp"


Channel::Channel(int id, std::string name, std::string newOperator, std::string newUser)
{
    _id = id;
    _name = name;
    _operators.push_back(newOperator);
    _userList.push_back(newUser);
}

Channel::Channel(int id, std::string name, std::string password, std::string newOperator, std::string newUser)
{
    _id = id;
    _name = name;
    _password = password;
    _operators.push_back(newOperator);
    _userList.push_back(newUser);
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
    _userList = other._userList;
}

Channel& Channel::operator=(const Channel &other)
{
    if (this != &other)
    {
        _id = other._id;
        _name = other._name;
        _password = other._password;
        _operators = other._operators;
        _userList = other._userList;
    }
    return (*this);
}

const std::string Channel::getName() const
{
    return (_name);
}

const std::vector<std::string> Channel::getUserList() const
{
    return (_userList);
}

const std::vector<std::string> Channel::getOperators() const
{
    return (_operators);
}