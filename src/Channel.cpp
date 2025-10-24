#include "Channel.hpp"

Channel::Channel()
{
}

Channel::~Channel()
{
}

Channel::Channel(const Channel &other)
{
    _name = other._name;
    _operators = other._operators;
    _userList = other._userList;
}

Channel& Channel::operator=(const Channel &other)
{
    if (this != &other)
    {
        _name = other._name;
        _operators = other._operators;
        _userList = other._userList;
    }
    return (*this);
}