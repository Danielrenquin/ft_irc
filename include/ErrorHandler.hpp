#pragma once
#include <string>
#include "Client.hpp"

/**
 * Classe utilitaire pour gérer tous les messages d'erreur IRC
 * Conforme à la RFC 1459 (Internet Relay Chat Protocol)
 * Toutes les méthodes sont statiques - pas besoin d'instanciation
 */
class ErrorHandler
{
    public:
        // 331 RPL_NOTOPIC
        static void noTopicSet(Client &cl, const std::string &channelTarget);

        // 401 ERR_NOSUCHNICK
        static void noSuchNick(Client& cl, const std::string& nickname);
        
        // 402 ERR_NOSUCHSERVER
        static void noSuchServer(Client& cl, const std::string& serverName);
        
        // 403 ERR_NOSUCHCHANNEL
        static void noSuchChannel(Client& cl, const std::string& channel);
        
        // 404 ERR_CANNOTSENDTOCHAN
        static void cannotSendToChan(Client& cl, const std::string& channel);
        
        // 405 ERR_TOOMANYCHANNELS
        static void tooManyChannels(Client& cl, const std::string& channel);
        
        // 406 ERR_WASNOSUCHNICK
        static void wasNoSuchNick(Client& cl, const std::string& nickname);
        
        // 407 ERR_TOOMANYTARGETS
        static void tooManyTargets(Client& cl, const std::string& target);
        
        // 409 ERR_NOORIGIN
        static void noOrigin(Client& cl);
        
        // 411 ERR_NORECIPIENT
        static void noRecipient(Client& cl, const std::string& command);
        
        // 412 ERR_NOTEXTTOSEND
        static void noTextToSend(Client& cl);
        
        // 413 ERR_NOTOPLEVEL
        static void noTopLevel(Client& cl, const std::string& mask);
        
        // 414 ERR_WILDTOPLEVEL
        static void wildTopLevel(Client& cl, const std::string& mask);
        
        // 421 ERR_UNKNOWNCOMMAND
        static void unknownCommand(Client& cl, const std::string& command);
        
        // 422 ERR_NOMOTD
        static void noMotd(Client& cl);
        
        // 423 ERR_NOADMININFO
        static void noAdminInfo(Client& cl, const std::string& server);
        
        // 424 ERR_FILEERROR
        static void fileError(Client& cl, const std::string& fileOp, const std::string& file);
        
        // 431 ERR_NONICKNAMEGIVEN
        static void noNicknameGiven(Client& cl);
        
        // 432 ERR_ERRONEUSNICKNAME
        static void erroneusNickname(Client& cl, const std::string& nick);
        
        // 433 ERR_NICKNAMEINUSE
        static void nicknameInUse(Client& cl, const std::string& nick);
        
        // 436 ERR_NICKCOLLISION
        static void nickCollision(Client& cl, const std::string& nick);
        
        // 441 ERR_USERNOTINCHANNEL
        static void userNotInChannel(Client& cl, const std::string& nick, const std::string& channel);
        
        // 442 ERR_NOTONCHANNEL x
        static void notOnChannel(Client& cl, const std::string& channel);
        
        // 443 ERR_USERONCHANNEL
        static void userOnChannel(Client& cl, const std::string& user, const std::string& channel);
        
        // 444 ERR_NOLOGIN
        static void noLogin(Client& cl, const std::string& user);
        
        // 445 ERR_SUMMONDISABLED
        static void summonDisabled(Client& cl);
        
        // 446 ERR_USERSDISABLED
        static void usersDisabled(Client& cl);
        
        // 451 ERR_NOTREGISTERED
        static void notRegistered(Client& cl);
        
        // 461 ERR_NEEDMOREPARAMS
        static void needMoreParams(Client& cl, const std::string& command);
        
        // 462 ERR_ALREADYREGISTRED
        static void alreadyRegistered(Client& cl);
        
        // 463 ERR_NOPERMFORHOST
        static void noPermForHost(Client& cl);
        
        // 464 ERR_PASSWDMISMATCH
        static void passwdMismatch(Client& cl);
        
        // 465 ERR_YOUREBANNEDCREEP
        static void youreBannedCreep(Client& cl);
        
        // 467 ERR_KEYSET
        static void keySet(Client& cl, const std::string& channel);
        
        // 471 ERR_CHANNELISFULL
        static void channelIsFull(Client& cl, const std::string& channel);
        
        // 472 ERR_UNKNOWNMODE
        static void unknownMode(Client& cl, const std::string& modeChar);
        
        // 473 ERR_INVITEONLYCHAN
        static void inviteOnlyChan(Client& cl, const std::string& channel);
        
        // 474 ERR_BANNEDFROMCHAN
        static void bannedFromChan(Client& cl, const std::string& channel);
        
        // 475 ERR_BADCHANNELKEY
        static void badChannelKey(Client& cl, const std::string& channel);

        static void ERR_BADCHANMASK(Client& cl, const std::string& channel);
        
        // 481 ERR_NOPRIVILEGES
        static void noPrivileges(Client& cl);
        
        // 482 ERR_CHANOPRIVSNEEDED
        static void chanOPrivsNeeded(Client& cl, const std::string& channel);
        
        // 483 ERR_CANTKILLSERVER
        static void cantKillServer(Client& cl);
        
        // 491 ERR_NOOPERHOST
        static void noOperHost(Client& cl);
        
        // 501 ERR_UMODEUNKNOWNFLAG
        static void uModeUnknownFlag(Client& cl);
        
        // 502 ERR_USERSDONTMATCH
        static void usersDontMatch(Client& cl);
        
        // ========== ALIAS POUR COMPATIBILITÉ ==========
        static void notEnoughParameters(Client& cl, const std::string& command);
        static void badChannelMask(Client& cl, const std::string& channel);
        static void notChannelOperator(Client& cl, const std::string& channel);
};