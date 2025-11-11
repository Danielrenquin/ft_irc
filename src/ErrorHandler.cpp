#include "ErrorHandler.hpp"
// 331 RPL_NOTOPIC
void ErrorHandler::noTopicSet(Client &cl, const std::string &channelTarget)
{
    cl.sendMessage("331 " + cl.getNickname() + " " + channelTarget + " :No topic is set");
}

// 401 ERR_NOSUCHNICK
void ErrorHandler::noSuchNick(Client& cl, const std::string& nickname)
{
    cl.sendMessage("401 " + nickname + " :No such nick/channel\r\n");
}

// 402 ERR_NOSUCHSERVER
void ErrorHandler::noSuchServer(Client& cl, const std::string& serverName)
{
    cl.sendMessage("402 " + serverName + " :No such server\r\n");
}

// 403 ERR_NOSUCHCHANNEL
void ErrorHandler::noSuchChannel(Client& cl, const std::string& channel)
{
    cl.sendMessage("403 " + channel + " :No such channel\r\n");
}

// 404 ERR_CANNOTSENDTOCHAN
void ErrorHandler::cannotSendToChan(Client& cl, const std::string& channel)
{
    cl.sendMessage("404 " + channel + " :Cannot send to channel\r\n");
}

// 405 ERR_TOOMANYCHANNELS
void ErrorHandler::tooManyChannels(Client& cl, const std::string& channel)
{
    cl.sendMessage("405 " + channel + " :You have joined too many channels\r\n");
}

// 406 ERR_WASNOSUCHNICK
void ErrorHandler::wasNoSuchNick(Client& cl, const std::string& nickname)
{
    cl.sendMessage("406 " + nickname + " :There was no such nickname\r\n");
}

// 407 ERR_TOOMANYTARGETS
void ErrorHandler::tooManyTargets(Client& cl, const std::string& target)
{
    cl.sendMessage("407 " + target + " :Duplicate recipients. No message delivered\r\n");
}

// 409 ERR_NOORIGIN
void ErrorHandler::noOrigin(Client& cl)
{
    cl.sendMessage("409 :No origin specified\r\n");
}

// 411 ERR_NORECIPIENT
void ErrorHandler::noRecipient(Client& cl, const std::string& command)
{
    cl.sendMessage("411 :No recipient given (" + command + ")\r\n");
}

// 412 ERR_NOTEXTTOSEND
void ErrorHandler::noTextToSend(Client& cl)
{
    cl.sendMessage("412 :No text to send\r\n");
}

// 413 ERR_NOTOPLEVEL
void ErrorHandler::noTopLevel(Client& cl, const std::string& mask)
{
    cl.sendMessage("413 " + mask + " :No toplevel domain specified\r\n");
}

// 414 ERR_WILDTOPLEVEL
void ErrorHandler::wildTopLevel(Client& cl, const std::string& mask)
{
    cl.sendMessage("414 " + mask + " :Wildcard in toplevel domain\r\n");
}

// 421 ERR_UNKNOWNCOMMAND x
void ErrorHandler::unknownCommand(Client& cl, const std::string& command)
{
    cl.sendMessage("421 " + command + " :Unknown command\r\n");
}

// 422 ERR_NOMOTD
void ErrorHandler::noMotd(Client& cl)
{
    cl.sendMessage("422 :MOTD File is missing\r\n");
}

// 423 ERR_NOADMININFO
void ErrorHandler::noAdminInfo(Client& cl, const std::string& server)
{
    cl.sendMessage("423 " + server + " :No administrative info available\r\n");
}

// 424 ERR_FILEERROR
void ErrorHandler::fileError(Client& cl, const std::string& fileOp, const std::string& file)
{
    cl.sendMessage("424 :File error doing " + fileOp + " on " + file + "\r\n");
}

// 431 ERR_NONICKNAMEGIVEN
void ErrorHandler::noNicknameGiven(Client& cl)
{
    cl.sendMessage("431 :No nickname given\r\n");
}

// 432 ERR_ERRONEUSNICKNAME
void ErrorHandler::erroneusNickname(Client& cl, const std::string& nick)
{
    cl.sendMessage("432 " + nick + " :Erroneus nickname\r\n");
}

// 433 ERR_NICKNAMEINUSE
void ErrorHandler::nicknameInUse(Client& cl, const std::string& nick)
{
    cl.sendMessage("433 " + nick + " :Nickname is already in use\r\n");
}

// 436 ERR_NICKCOLLISION
void ErrorHandler::nickCollision(Client& cl, const std::string& nick)
{
    cl.sendMessage("436 " + nick + " :Nickname collision KILL\r\n");
}

// 441 ERR_USERNOTINCHANNEL
void ErrorHandler::userNotInChannel(Client& cl, const std::string& nick, const std::string& channel)
{
    cl.sendMessage("441 " + nick + " " + channel + " :They aren't on that channel\r\n");
}

// 442 ERR_NOTONCHANNEL
void ErrorHandler::notOnChannel(Client& cl, const std::string& channel)
{
    cl.sendMessage("442 " + channel + " :You're not on that channel\r\n");
}

// 443 ERR_USERONCHANNEL
void ErrorHandler::userOnChannel(Client& cl, const std::string& user, const std::string& channel)
{
    cl.sendMessage("443 " + user + " " + channel + " :is already on channel\r\n");
}

// 444 ERR_NOLOGIN
void ErrorHandler::noLogin(Client& cl, const std::string& user)
{
    cl.sendMessage("444 " + user + " :User not logged in\r\n");
}

// 445 ERR_SUMMONDISABLED
void ErrorHandler::summonDisabled(Client& cl)
{
    cl.sendMessage("445 :SUMMON has been disabled\r\n");
}

// 446 ERR_USERSDISABLED
void ErrorHandler::usersDisabled(Client& cl)
{
    cl.sendMessage("446 :USERS has been disabled\r\n");
}

// 451 ERR_NOTREGISTERED
void ErrorHandler::notRegistered(Client& cl)
{
    cl.sendMessage("451 :You have not registered\r\n");
}

// 461 ERR_NEEDMOREPARAMS x
void ErrorHandler::needMoreParams(Client& cl, const std::string& command)
{
    cl.sendMessage("461 " + command + " :Not enough parameters\r\n");
}

// 462 ERR_ALREADYREGISTRED
void ErrorHandler::alreadyRegistered(Client& cl)
{
    cl.sendMessage("462 :You may not reregister\r\n");
}

// 463 ERR_NOPERMFORHOST
void ErrorHandler::noPermForHost(Client& cl)
{
    cl.sendMessage("463 :Your host isn't among the privileged\r\n");
}

// 464 ERR_PASSWDMISMATCH
void ErrorHandler::passwdMismatch(Client& cl)
{
    cl.sendMessage("464 :Password incorrect\r\n");
}

// 465 ERR_YOUREBANNEDCREEP
void ErrorHandler::youreBannedCreep(Client& cl)
{
    cl.sendMessage("465 :You are banned from this server\r\n");
}

// 467 ERR_KEYSET
void ErrorHandler::keySet(Client& cl, const std::string& channel)
{
    cl.sendMessage("467 " + channel + " :Channel key already set\r\n");
}

// 471 ERR_CHANNELISFULL
void ErrorHandler::channelIsFull(Client& cl, const std::string& channel)
{
    cl.sendMessage("471 " + channel + " :Cannot join channel (+l)\r\n");
}

// 472 ERR_UNKNOWNMODE
void ErrorHandler::unknownMode(Client& cl, const std::string& modeChar)
{
    cl.sendMessage("472 " + modeChar + " :is unknown mode char to me\r\n");
}

// 473 ERR_INVITEONLYCHAN
void ErrorHandler::inviteOnlyChan(Client& cl, const std::string& channel)
{
    cl.sendMessage("473 " + channel + " :Cannot join channel (+i)\r\n");
}

// 474 ERR_BANNEDFROMCHAN
void ErrorHandler::bannedFromChan(Client& cl, const std::string& channel)
{
    cl.sendMessage("474 " + channel + " :Cannot join channel (+b)\r\n");
}

// 475 ERR_BADCHANNELKEY
void ErrorHandler::badChannelKey(Client& cl, const std::string& channel)
{
    cl.sendMessage("475 " + channel + " :Cannot join channel (+k)\r\n");
}

// 476 x
void ErrorHandler::ERR_BADCHANMASK(Client& cl, const std::string& channel)
{
     cl.sendMessage("476 " + channel + " :Bad Channel Mask\r\n");
}

void ErrorHandler::notEnoughParameters(Client& cl, const std::string& command)
{
    // 461 <command> :Not enough parameters
    cl.sendMessage("461 " + cl.getNickname() + " " + command + " :Not enough parameters\r\n");
}

void ErrorHandler::notChannelOperator(Client& cl, const std::string& channel)
{
    // 482 <channel> :You're not channel operator
    cl.sendMessage("482 " + cl.getNickname() + " " + channel + " :You're not channel operator\r\n");
}


// 481 ERR_NOPRIVILEGES
void ErrorHandler::noPrivileges(Client& cl)
{
    cl.sendMessage("481 :Permission Denied- You're not an IRC operator\r\n");
}

// 482 ERR_CHANOPRIVSNEEDED
void ErrorHandler::chanOPrivsNeeded(Client& cl, const std::string& channel)
{
    cl.sendMessage("482 " + channel + " :You're not channel operator\r\n");
}

// 483 ERR_CANTKILLSERVER
void ErrorHandler::cantKillServer(Client& cl)
{
    cl.sendMessage("483 :You cant kill a server!\r\n");
}

// 491 ERR_NOOPERHOST
void ErrorHandler::noOperHost(Client& cl)
{
    cl.sendMessage("491 :No O-lines for your host\r\n");
}

// 501 ERR_UMODEUNKNOWNFLAG
void ErrorHandler::uModeUnknownFlag(Client& cl)
{
    cl.sendMessage("501 :Unknown MODE flag\r\n");
}

// 502 ERR_USERSDONTMATCH
void ErrorHandler::usersDontMatch(Client& cl)
{
    cl.sendMessage("502 :Cant change mode for other users\r\n");
}
