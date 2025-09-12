# IRC Server

<p align="center">
    <img src="https://github.com/alx-sch/IRC_server/blob/main/.assets/ft_irc_badge.png" alt="ft_irc_badge.png" />
</p>

This project is about building a functional IRC (Internet Relay Chat) server from scratch in C++. The goal is to create a multi-client, event-driven network application that handles IRC protocol commands and manages user connections, channels, and message routing.

This project is a collaboration between:

- **[Patrícia](https://github.com/paribeir)**: TASKS TDB
- **[Pavlos](https://github.com/sysex89)**: TASKS TDB
- **[Alex](https://github.com/alx-sch)**: TASKS TDB
  
---

## Table of Contents
  
- [What is IRC?](#what-is-irc)
- [How to Build the Server](#how-to-build-the-server)
- [How to Connect to the Server](#how-to-connect-to-the-server)
- [Server Features](#server-features)
- [Client–Server Communication](#clientserver-communication)
- [Project Architecture](#project-architecture)
- [Core Functionality](#core-functionality)
- [Core Server Functions](#core-server-functions)
  
---

## What is IRC?

**IRC**, or **Internet Relay Chat**, is an open protocol for real-time text-based communication. It was created in **1988** by Jarkko Oikarinen in Finland, with the protocol officially documented in [**RFC 1459**](https://www.rfc-editor.org/rfc/rfc1459) in 1993 for the first time<sup><a href="#footnote1">[1]</a></sup>. This project's server implementation is based on this foundational RFC.

Think of IRC as the standard for instant messaging and group chat that existed before web browsers became the primary application for accessing the internet.

- **How it works:** IRC operates on a client-server model. Users run a client program that connects to an IRC server. These servers are interconnected in a network to form an entire IRC network.
- **Channels and Users:** Communication happens in channels (group chats) or through private, one-on-one messages. A user is identified by a unique nickname on a given network.
- **Key Features:** IRC is known for its simplicity and efficiency. It's a lightweight protocol, making it ideal for large-scale communities and for sharing information quickly, without the overhead of modern web-based applications. It's still used by open-source projects, developers, and niche communities. A popular GUI client is [**HexChat**](https://hexchat.github.io/), while [**WeeChat**](https://weechat.org/) is a terminal client

### Key Terms
- **Server:** A program that manages connections and routes messages between users.
- **Client:** A program (like mIRC, HexChat, or WeeChat) that a user runs to connect to a server.
- **Channel:** A named group chat room, denoted by a prefix like `#` or `&`. For example, `#42chat`.
- **Nickname:** A unique name a user chooses to identify themselves on a network.
- **Hostmask:** The full identity of a user, typically `nickname!username@hostname`, used for security and access control.

---

## How to Build the Server

**1. Prerequisites**

To build and run the server, ensure you have the following tools installed on your system:

- **Git:** To clone the source code.
- A **C++ Compiler** (e.g., `g++`): To compile the code.
- **Make:** A build automation tool that reads the `Makefile`.

**2. Clone the Repository**  

   Clone the repository and navigate into the project directory:
   ```
   git clone https://github.com/alx-sch/IRC_server ircserv && cd ircserv
   ```
**3. Build the Server Program**  

   Use the `make` command to compile the server executable.
   ```
   make
   ```
   This command reads the project's `Makefile` and handles the entire compilation process. The build system is designed to be **cross-platform** and automatically adapts for **macOS** and **Linux**, which use **POSIX-compliant** sockets. **Windows** is not supported as its Winsock API is incompatible with this approach.

   - **OS-Specific Build:** The server's non-blocking socket functionality is handled differently on Linux and macOS. The `Makefile` detects the OS and passes the correct compiler flags to handle these variations.
   - **Linux:** The code uses the `SOCK_NONBLOCK` flag.
   - **macOS:** The code uses the `fcntl()` function to set the non-blocking flag.
     
**4. Run the Server**  

   Provide a port number and a password for the server. The standard IRC port is `6667`.
   ```
   // Usage: ./ircserv <port> <password>
   ./ircserv 6667 pw123
   ```
   If you want to allow connections without a password, use an empty string (`"`) as the password argument.

**5. `make` Commands**

While `make` is sufficient for a basic build, here are a few other essential commands you might use:

 - `make clean`: Removes all the compiled object files (`.o` and `.d` files) and the obj directory.
 - `make fclean`: Performs a full clean by removing both the object files and the final `ircserv` executable.
 - `make re`: A shortcut that runs `fclean` and then `all` to completely rebuild the project from scratch.
 - `make checkout_os`: Prints the detected operating system to the console.

---

## How to Connect to the Server

After you have built and run the server, you can connect to it using a variety of clients.

**1. First, Find Your IP Address**

To connect, you need to know your machine's IP address. Open a terminal and use this command to list your local IP addresses.
- Linux
	```
	$ hostname -I
	10.14.6.2 172.17.0.1 192.168.122.1
	```

 - macOS
   ```
   $ ifconfig | grep "inet "
   inet 127.0.0.1 netmask 0xff000000
   inet 192.168.2.152 netmask 0xffffff00 broadcast 192.168.2.255
   ```

For a local connection (from your own machine), you can always use the loopback address `127.0.0.1` or just `localhost`.   

To make your server **publicly accessible**, you will need to configure your router with **port forwarding**, directing a public port (e.g., `6667`) to your machine's local IP address. [This Youtube video](https://www.youtube.com/watch?v=PbV8gPi_RSo&ab_channel=DrewHowdenTech) might give you some insights on how to do this.

**2. Connect with Terminal Tools**

To quickly test the server, you can use `nc` (Netcat) or `telnet`.   

```
nc <your-IP-address> <port>
```

or

```
telnet <your-IP-address> <port>
```

Example: `nc localhost 6667` (accessing from the same machine that hosts the server) or `nc 10.14.6.2 6667` (accessing from the same local network, e.g. same host machine or some other computer in the same local network).

Once connected, you can communicate with the server using IRC commands (you would usually first register with the server by sending valid `NICK`, `USER`, and `PASS` commands).

**3. Connect with a GUI Client**

For a better user experience, a graphical client is recommended, e.g. Hexchat:

- Open HexChat. In the Network List (prompted automatically), add a new network **(1)**.

	<p align="center">
	    <img src="https://github.com/alx-sch/IRC_server/blob/main/.assets/HexChat_01.png" alt="HexChat_01"  width="300" />
	</p>

- Name the new network **(1)** and edit its settings **(2)**:

	<p align="center">
	    <img src="https://github.com/alx-sch/IRC_server/blob/main/.assets/HexChat_02.png" alt="HexChat_02"  width="300" />
	</p>

- There is already a default server listed. Change its host IP address and port **(1)** to match your own. Disable SSL, since the server does not support it (otherwise it would fail to interpret the encrypted requests) **(2)**. Enter the server password if required **(3)**. Close the network settings **(4)**:

	<p align="center">
	    <img src="https://github.com/alx-sch/IRC_server/blob/main/.assets/HexChat_03.png" alt="HexChat_03"  width="300" />
	</p>

- Enter the user info (nick, alt nick, username) **(1)**, select the custom network **(2)**, and connect **(3)**.

	<p align="center">
	    <img src="https://github.com/alx-sch/IRC_server/blob/main/.assets/HexChat_04.png" alt="HexChat_04"  width="300" />
	</p>

- HexChat automatically sends the registration commands (`NICK`, `USER`, `PASS`) to the server. The client is now ready: join a channel with `/join #channelname` or send a private message with `/msg nickname message`.

	<p align="center">
	    <img src="https://github.com/alx-sch/IRC_server/blob/main/.assets/HexChat_rdy.png" alt="HexChat_rdy"  width="450" />
	</p>

- The server logs events to the console and also saves them to log files in ---_>>  XXX (dedicated folder??). <<<---- The client may send commands the server does not support (e.g. `CAP`), but all core IRC functions still work (see below).

	<p align="center">
	    <img src="https://github.com/alx-sch/IRC_server/blob/main/.assets/server_log.png" alt="server_log"  width="450" />
	</p>

---

## Server Features

- User Commands:

	- `NICK`: Handles setting or changing a nickname - `NICK newnickname`
	- `USER`: Handles setting a username (and other info) - `USER <username> <hostname> <servername> <realname>` → `USER guest 0 * :Ronnie Reagan`. Hostname and servername are usually ignored/masked in modern IRC but info is used to form the  hostmask `nickname!username@hostname`, which uniquely identifies a client.
	- `PASS`: Handles the connection password for authentication - `PASS mysecretpassword`
	- `JOIN`: Allows a user join a channel, or create it if it doesn’t exist - `JOIN #general`
	- `QUIT`: Allows a user to disconnect from the server - `QUIT :Leaving for lunch` (reason is optional)
	- `PART`: Allows a user to leave a channel - `PART #oldchannel`
	- `PRIVMSG`: Used for sending private messages to a user or a channel - `PRIVMSG username :Hello there!`, `PRIVMSG #general :What's everyone up to?`
 	- `NOTICE`: Similar to `PRIVMSG`, but used for server messages and automated responses. It should not be used for client-to-client communication. The main difference is that a user's IRC client should never automatically respond to a `NOTICE` - `NOTICE username :You have a new message.`

- Channel Operator Commands:
  The server differentiates between operators and regular users. Operators have the authority to use specific commands to manage a channel:

	- `KICK`: Ejects a client from a channel - `KICK #general baduser :You've been kicked for spamming.` (reason is optional)
	- `INVITE`: Invites a client to a channel. This is particularly useful for invite-only channels - `INVITE newuser #general`
	- `TOPIC`: Changes or views the channel topic - `TOPIC #general :Welcome to the main chat!` (setting topic), `TOPIC #general` (checking topic)
	- `MODE`: Changes a channel's mode using the following flags, using `+` or `-` to add/remove these modes.
		- `i`: Toggles the invite-only channel mode - `MODE #private +i` (makes channel invite-only), `MODE #private -i` (removes invite-only restriction)
		- `t`: Toggles the restriction of the `TOPIC` command to channel operators - `MODE #general +t` restricts the `TOPIC` command so only operators can change the topic.
		- `k`: Toggles the channel key (password) - `MODE #locked +k secretkey`
		- `o`: Gives or takes away channel operator privilege  - `MODE #general +o newoperator`
		- `l`: Sets or removes a user limit for the channel - `MODE #limited +l 10`

---

## Client–Server Communication

### User Registration

User registration on an IRC server is a three-step process: **password** (`PASS`), **nickname** (`NICK`), and **user information** (`USER`). The client sends these commands to the server and the server validates them to register the user and begin the communication session.    

Upon successful registration, the server sends back welcome messages with numeric codes `001` through `004` to confirm the connection and provide server details.

```text
:42ircRebels.net 001 nick :Welcome to the 42 IRC Network, nick!user@host
:42ircRebels.net 002 nick :Your host is 42ircRebels.net, running version eval-42.42
:42ircRebels.net 003 nick :This server was created Thu Sep 11 2025 at 07:30:01 UTC
:42ircRebels.net 004 nick 42ircRebels.net eval-42.42 - itkol
```
              
### Server Replies to Client

In IRC, the server communicates with clients by sending **protocol-compliant replies**. Each message follows a standard format, as specified in RFC 1459<sup><a href="#footnote1">[1]</a></sup>.

#### IRC Message Format

Every message from the server to the client has this general structure:

```text
:<prefix> <command or numeric> <target> :<message>\r\n
```

| Field                  | Description                                                                 |
|------------------------|-----------------------------------------------------------------------------|
| `:<prefix>`            | Always the server name (e.g. `irc.example.org`)                             |
| `<command>` or `<numeric>` | Either a command name (e.g. `NOTICE`) or a numeric reply (`001`, `433`, `464`, etc.) |
| `<target>`             | Typically the user's nickname, or `*` if the client is not yet registered   |
| `:<message>`           | Human-readable message text (trailing parameter)                            |
| `\r\n`                 | Required line ending in all IRC messages                                    |

#### Function: `sendReply()`

The following helper method of the `User` class ensures the reply is properly formatted and sent:

```cpp
void User::sendReply(const std::string& message)
{
	std::string fullMessage = ":" + serverName + " " + message + "\r\n";
	send(_fd, fullMessage.c_str(), fullMessage.length(), 0);
}
```

This function:

 - Prepends the server name as the prefix
 - Appends `\r\n` to comply with the IRC protocol
 - Uses `send()` to transmit the message to the client’s socket

Example usage:

```cpp
user->sendReply("001 nick :Welcome to the IRC Network, nick!user@host");
```

Which sends the following to the client

```text
:42ircRebels.net 001 nick :Welcome to the 42 IRC Network, nick!user@host
```

---

## Project Architecture

The core of the server is the `Server` class. It's responsible for managing the entire IRC network, including user connections, channels, and command handling. It uses a **non-blocking, single-threaded architecture** to manage multiple clients efficiently via I/O multiplexing with `select()`.

The project is structured around several key classes:

- **`Server`**: The central class that manages the main server socket, new connections, and the main server loop. It contains maps to store and manage `User` and `Channel` objects.

- **`User`**: Represents an individual client connected to the server. It stores all user-specific data, such as nickname, username, and connection status, and has input/output buffers for network communication. A user can be in multiple channels, and the `User` class tracks this membership.

- **`Channel`**: Represents a chat room on the server. It manages its own list of members, operators, invitations, topic, and channel modes (e.g., password, invite-only, user limit).

- **`Command`**: A static utility class responsible for parsing and handling all IRC commands. It uses a `tokenize()` method to break down incoming messages and dispatches them to specific handler functions (e.g., `handleJoin`, `handleKick`).

---

## Core Functionality

1. **Server Lifecycle:** The server starts in `main.cpp` by creating a `Server` instance with a given port and password. The `Server::run()` method then starts a loop that continuously monitors all client sockets using `select()`. It waits for one of three types of events to handle I/O:
    - **New Connections:** A new connection request on the main server socket is handled by `FD_ISSET(_fd, &readFds)`.
    - **Incoming Data:** Incoming data from existing clients is processed by `handleReadyUsers(readFds)`.
    - **Outgoing Data:** Outgoing data is sent to clients with pending messages by `handleWriteReadyUsers(writeFds)`.
      
2. **User Registration:** A new user must complete a three-step registration process using the `PASS`, `NICK`, and `USER` commands. The `User` class tracks the status of these commands, and the `tryRegister()` method attempts to complete the registration once all three commands have been successfully processed. The server also validates the nickname according to IRC rules to prevent invalid or duplicate nicknames.
   
3. **Command Processing:**
    - When a full message is received from a client, the `Command::tokenize()` function parses the message into a list of tokens.
    - `Command::getCmd()` determines the command type.
    - `Command::handleCommand()` then calls the appropriate static handler function (e.g., `handleJoin` for the `JOIN` command).
  
4. **Channel Management:** The `Server` class manages all channels, with the `Channel` class handling channel-specific details. The server supports a variety of channel-related commands, including:
    - `JOIN`: Allows a user to join a channel, with checks for passwords (`+k`), user limits (`+l`), and invite-only status (`+i`). If the channel doesn't exist, it is created.
    - `PART`: A user leaves a channel.
    - `KICK`: An operator can forcibly remove another user from a channel.
    - `TOPIC`: Sets or retrieves a channel's topic, with optional operator-only protection.
    - `INVITE`: An operator can invite a user to an invite-only channel.
  
5. **Data Flow:** The server uses input and output buffers for each `User`. Incoming data from a client is accumulated in the input buffer until a complete IRC message (`\r\n`) is found. Once processed, a response is formatted and appended to the user's output buffer, which is then sent back to the client when their socket is ready for writing. This buffering prevents the server from blocking while waiting to send data.

----  

## Core Server Functions

-  **`socket()`:** `Server::createSocket()` uses `socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0)` to create the server socket. The `SOCK_STREAM` specifies a TCP socket, and `AF_INET` sets the address family to IPv4. The `SOCK_NONBLOCK` flag is an important part, as it makes the socket non-blocking.
Making a socket non-blocking means that system calls like `recv()` or `send()` will not pause the program if an operation cannot be completed immediately. Instead of waiting for data to arrive or for the network buffer to clear, the call will return immediately with an error, typically `EAGAIN` or `EWOULDBLOCK`.

- **`setsockopt()`:** In `Server::setSocketOptions()`, this function is used to set the `SO_REUSEADDR` option. This allows the server to restart immediately on the same port without waiting for the operating system to clear the previous socket's state.

-  **`bind()`:** `Server::bindSocket()` uses `bind()` to attach the server socket to a specific address and port. It uses `INADDR_ANY` to bind to all available network interfaces (local machine, local network, public Internet, etc.) and `htons(_port)` to ensure the port number is in the correct network byte order.

-  **`listen()`:** `Server::startListening()` method calls `listen()` to put the socket into a state where it waits for incoming connections. The `SOMAXCONN` constant is used as the backlog, which tells the operating system how many incoming connections can be queued up while the server is busy.

-  **`select()`:** `Server::run()` uses `select()` as the central mechanism for its event loop. This function is what allows the server to monitor all sockets at once for incoming messages or readiness to send data. The `prepareReadSet()` and `prepareWriteSet()` functions are specifically written to work with `select()`.

- **`accept()`:** In `Server::acceptNewUser()`, the `accept()` call is used to create a new socket for an incoming connection. This new socket is then used to communicate with the specific client.

- **`send()`:** `Server::handleWriteReadyUsers()` uses `send()` to push data from a user's output buffer to their connected socket.

- **`recv()`:** `Server::handleUserInput()` uses `recv()` to read data from a user's socket into a buffer. It checks the number of bytes read to determine if the client is still connected or if a message has been received.
  
---

## Commands to Implement

 Command: KICK
 `Parameters: <channel> <user> [<comment>]`
 https://www.rfc-editor.org/rfc/rfc1459#section-4.2.8
 
 Command: INVITE
 `Parameters: <nickname> <channel>`
 https://www.rfc-editor.org/rfc/rfc1459#section-4.2.7

 Command: TOPIC
 `Parameters: <channel> [<topic>]`
 https://www.rfc-editor.org/rfc/rfc1459#section-4.2.4
 
 Command: MODE
  `Parameters: <channel> {[+|-]|o|p|s|i|t|n|b|v} [<limit>] [<user>]`
               `[<ban mask>]`
OR
` Parameters: <nickname> {[+|-]|i|w|s|o}`
https://www.rfc-editor.org/rfc/rfc1459#section-4.2.3
 
 Command: LIST <---- NOT REQUIRED BY SUBJECT
 `Parameters: [<channel>{,<channel>} [<server>]]`
 https://www.rfc-editor.org/rfc/rfc1459#section-4.2.6

---

## References

<a name="footnote1">[1]</a> Oikarinen, J.; Reed, D.(1993). *Internet Relay Chat Protocol*. [Request for Comments: 1459](https://www.rfc-editor.org/rfc/rfc1459)
