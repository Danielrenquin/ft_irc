#pragma once

#include "Server.hpp"

/*USER <username> <hostname> <servername> <realname>
```

**Les 4 paramètres :**

1. **`<username>`** = `alice` → votre nom d'utilisateur
2. **`<hostname>`** = `0` → le nom d'hôte de votre machine
3. **`<servername>`** = `*` → le serveur sur lequel vous êtes
4. **`<realname>`** = `Alice Johnson` → votre vrai nom

**Pour le `0` (hostname) et `*` (servername) :**

Le hostname et servername sont normalement ignorés par le serveur IRC quand la commande USER vient d'un client directement connecté (pour des raisons de sécurité), mais ils sont utilisés dans la communication serveur-à-serveur

**Donc en pratique :**

- `0` est simplement une valeur fictive pour le hostname (peut être n'importe quoi, le serveur l'ignore)
- `*` signifie "n'importe quel serveur" ou "serveur par défaut" (valeur conventionnelle)

**Vous pouvez aussi utiliser :**
```
USER alice localhost irc.example.com :Alice Johnson
USER alice myhost.com * :Alice Johnson
USER alice 0 0 :Alice Johnson*/

class Server;


class Client
{
	private:
		std::string _type;
		int _clientFd;
		pollfd *_pfd; // normalement stocké sur le serveur
    protected:
        std::string _username; //devra faire 9 charactères
		std::string _nickname;
        std::string _hostname;
		std::string _servername;
        std::string _realname;
        //std::vector<channel> _channel
        //int _port; //port utiliser côté client
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
		int getFd() const;

		void sendMessage(const std::string &msg) const;


        //void join();
        //see_channels(); ???

        /*addclient
        removeclient
        msg/n/r*/
};


 // Operateur cree un channel
 // Commande join pour client
 // Parsing client pour son instance
 // Commmencer la classe channel
