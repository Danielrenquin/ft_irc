# ft_irc

An Internet Relay Chat (IRC) server written in **C++98**. Clients connect over TCP, register with a password, nickname and username, then join channels and exchange messages in real time.

The project follows the 42 *ft_irc* subject: a single-process server, non-blocking I/O, and multiplexing with `poll()`.

## Requirements from the subject

- C++98 only, compiled with `-Wall -Wextra -Werror`
- Executable name: `ircserv`
- Usage: `./ircserv <port> <password>`
- Handle several clients at once without blocking
- Non-blocking file descriptors (`fcntl` may only be used with `F_SETFL` and `O_NONBLOCK`)
- A single `poll()` (or equivalent) for all I/O: listen, read and write
- No `fork()`
- Compatible with a real IRC client (for example Irssi)
- Authentication with the server password
- Channels, operators, and the mandatory operator commands
- Partial IRC compatibility with RFC 1459 / RFC 2812 (numeric replies)



## Build

```bash
make
```

Clean object files:

```bash
make clean
```

Remove objects and the binary:

```bash
make fclean
```

Rebuild from scratch:

```bash
make re
```



## Run

```bash
./ircserv <port> <password>
```

Example:

```bash
./ircserv 6667 42
```

Then connect with Irssi, HexChat, or netcat:

```bash
nc localhost 6667
```

A helper script `script.sh` can open the server and three netcat clients in separate terminals (default port `6667`, password `42`). Compile first with `make`.

## Registration

A client must complete registration before using channel commands:


| Command                           | Role                                                 |
| --------------------------------- | ---------------------------------------------------- |
| `PASS <password>`                 | Server password (must match the one given at launch) |
| `NICK <nickname>`                 | Choose a unique nickname                             |
| `USER <username> 0 * :<realname>` | Set identity and finish registration                 |


Typical netcat sequence:

```
PASS 42
NICK alice
USER alice 0 * :Alice
```



## Commands



### Messaging and session


| Command                    | Description                                       |
| -------------------------- | ------------------------------------------------- |
| `PRIVMSG <target> :<text>` | Send a private message to a nickname or a channel |
| `QUIT [:<reason>]`         | Disconnect and notify channels                    |




### Channels


| Command                                    | Description                                            |
| ------------------------------------------ | ------------------------------------------------------ |
| `JOIN <#channel>[,#other] [<key>[,<key>]]` | Join or create a channel                               |
| `KICK <#channel> <nick> [:<reason>]`       | Operator: remove a user from a channel                 |
| `INVITE <nick> <#channel>`                 | Operator: invite a user (needed when mode `+i` is set) |
| `TOPIC <#channel> [:<topic>]`              | View or set the topic (restricted if mode `+t`)        |
| `MODE <#channel> {[+                       | -]itkol} [params]`                                     |




### Channel modes


| Mode | Meaning                             |
| ---- | ----------------------------------- |
| `i`  | Invite-only                         |
| `t`  | Only operators can change the topic |
| `k`  | Channel key (password)              |
| `o`  | Give or take operator privilege     |
| `l`  | User limit                          |


Examples:

```
JOIN #general
PRIVMSG #general :hello
MODE #general +i
INVITE bob #general
MODE #general +k secret
MODE #general +l 10
TOPIC #general :welcome
KICK #general bob :spam
```



## Project layout

```
include/     Headers (Server, Client, Channel, CommandHandler, ErrorHandler)
src/         Implementation
Makefile     C++98 build
script.sh    Local multi-client test helper
```


| Class            | Role                                                                     |
| ---------------- | ------------------------------------------------------------------------ |
| `Server`         | Socket setup, `poll()` loop, clients, channels, `PASS` / `NICK` / `USER` |
| `Client`         | Connected user (nickname, username, fd)                                  |
| `Channel`        | Members, operators, topic, modes, key, limit, invites                    |
| `CommandHandler` | Dispatch of `JOIN`, `KICK`, `PRIVMSG`, `QUIT`, `INVITE`, `TOPIC`, `MODE` |
| `ErrorHandler`   | RFC numeric error and reply messages                                     |




## Constraints (subject)

- Do not crash (segfault, abort, uncaught exception, and so on)
- Do not leak file descriptors
- Do not use extra network libraries; POSIX sockets only
- Communication is TCP/IP



## Resources

- [RFC 1459 — Internet Relay Chat Protocol](https://www.rfc-editor.org/rfc/rfc1459)
- [RFC 2812 — IRC Client Protocol](https://www.rfc-editor.org/rfc/rfc2812)


## Authors

[lmonsat](https://profile.intra.42.fr/users/lmonsat)
[drenquin](https://profile.intra.42.fr/users/drenquin)

## 42 School

This project was developed as part of the school's curriculum (https://42luxembourg.lu/).


