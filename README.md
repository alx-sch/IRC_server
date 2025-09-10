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

## How to Setup the Server

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
**3. Build the server program**  

   Use the `make` command to compile the server executable.
   ```
   make
   ```
   This command reads the project's `Makefile` and handles the entire compilation process. The build system is designed to be **cross-platform**, automatically adjusting for the operating system.

   - **OS-Specific Build:** The server's non-blocking socket functionality is handled differently on Linux and macOS. The `Makefile` detects the OS and passes the correct compiler flags to handle these variations.
   - **Linux:** The code uses the `SOCK_NONBLOCK` flag.
   - **macOS:** The code uses the `fcntl()` function to set the non-blocking flag.
     
**4. Run the server**  

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

Example: `nc 127.0.0.1 6667` (accessing from the same machine that hosts the server) or `nc 10.14.6.2 6667` (accessing from the same local network, e.g. same host machine or some other computer in the same local network).

Once connected, you can communicate with the server using IRC commands (you would usually first register with the server by sending valid `NICK`, `USER`, and `PASS` commands).

**3. Connect with a GUI Client**

For a better user experience, a graphical client is recommended, e.g. Hexchat:

- Open HexChat and go to Network List.

<p align="center">
    <img src="https://github.com/alx-sch/IRC_server/blob/main/.assets/HexChat_01.png" alt="HexChat_01" />
</p>


- Add a new network and set the server address to <your-IP-address>/<port>.

Enter your desired nickname and username, then click Connect.

Once connected, you can join a channel by typing /join #channelname in the chat window.

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

## Networking Basics

To understand how an IRC server functions at a technical level, it's essential to first grasp a few core networking concepts. This project is built from the ground up to handle network communication, and understanding these principles will provide insight into the server's design and functionality.

- **IP Address**
    - Identifies a **host/device** on a network.
    - IPv4 example: `192.168.1.5`, `127.0.0.1`
    - IPv6 example: `::1`
    - Used to route data to the correct machine.
    - `localhost` is a special hostname referring to the **local machine** (your own computer), typically used for testing or internal communication.
      It resolves to IP `127.0.0.1` (IPv4) or `::1` (IPv6).
    - More info on IP addresses [here](https://github.com/alx-sch/NetPractice?tab=readme-ov-file#ip-address-network-layer).

- **Port**
    - Identifies a specific **service or application** on a host.
    - Range: `1`–`65535` (unsigned 16-bit integer); port `0` is reserved as a wildcard, allowing the OS to assign an ephemeral (random) port automatically.
    - Common ports: `22` (SSH), `80` (HTTP), **`6667` (default for IRC)**

- **Socket**
    - A **socket** is a combination of an IP address and a port number.
    - It represents one endpoint of a network communication channel.
    - For two processes to talk (e.g. client ↔ server), **each process uses its own socket**: one local, one remote.
    - Example format: `127.0.0.1:6667`

- **Client/Server Model**
    - A **server** waits for incoming connections (e.g. our IRC server).
    - A **client** initiates the connection (e.g. IRC user, `telnet`, `netcat`).
    - The server can handle multiple clients simultaneously.
    - The server listens on a **well-known** port, the client uses an **ephemeral** port (temporary client-side ports).

- **Blocking vs Non-blocking I/O**
    - Blocking I/O:
        - The program waits ("blocks") until a read/write finishes.
        - Simple but inefficient for handling many clients.
    - Non-blocking I/O:
        - The program can check for input/output without getting stuck.
        - Essential for writing responsive servers
        - Requires checking readiness (I/O multiplexing, see below).

- **I/O Multiplexing (select, poll)**
    - This is a technique that allows a single-threaded server to monitor multiple sockets at once without blocking.
    - `select()` and `poll()` are system calls used to wait for activity on multiple file descriptors (which represent sockets):
        - They monitor client connections.
        - They check which sockets are ready to read or write.
    - These tools are used to build efficient, scalable servers without the overhead of creating a separate thread or process for every client.

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

## Client–Server Communication

### User Registration (sent by client upon server connection)
When a user connects to an IRC server via a client, they register by sending a specific sequence of commands to be recognized as a valid user.

1. `PASS <password>`
    - Optional, but must come first.
    - If the server requires a password and it is not sent or incorrect, it replies with `464 ERR_PASSWDMISMATCH` and closes the connection.
    - If `PASS` is sent after `NICK` or `USER`, the server replies with `462 ERR_ALREADYREGISTRED`.
      
2. `NICK <nickname>`   
     - Sets the user's nickname.
     - The nickname must be unique and valid (e.g. no spaces, limited symbols).
     - If the nickname is already in use, the server replies with `433 ERR_NICKNAMEINUSE`.

3. `USER <username> <hostname> <servername> :<realname>`
    - Despite four fields, only `username` and `realname` are used in modern IRC servers.
    - `hostname` and `servername` are obsolete and typically ignored.
    - The  `realname ` must be prefixed with a colon ( `:`), because everything after it is treated as a single token, even if it contains spaces.

Once the server receives all required fields ( `NICK `, `USER `, and  `PASS ` if required), the client is marked as registered, and the server typically
responds with welcome messages (numeric replies  `001` through  `004`), for example:

```text
001 Alex :Welcome to the IRC Network, Alex!alex@your-ip
002 Alex :Your host is irc.example.org, running version ircd-1.0
003 Alex :This server was created Fri Jul 19 2024 at 15:00:00 UTC
004 Alex irc.example.org ircd-1.0 aiowrs
```

| Code | Meaning                                                                 |
|------|-------------------------------------------------------------------------|
| 001  | Welcome message – confirms the nickname and shows your identity (`nick!username@host`) |
| 002  | Server hostname and version                                             |
| 003  | Server creation date/time                                               |
| 004  | Server name, version, supported user and channel modes                 |

### Server Replies to Client

In IRC, the server communicates with clients by sending **protocol-compliant replies**. Each message follows a standard format, as specified in [RFC 1459](https://datatracker.ietf.org/doc/html/rfc1459).

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

Example:

```text
:irc.example.org 001 Alex :Welcome to the IRC Network, Alex!alex@host
```

Before the client has sent both `NICK` and `USER`, there is no known nickname. In that case, IRC uses `*` as the target:

```text
:irc.example.org 464 * :Password incorrect
```

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
user->sendReply("001 Alex :Welcome to the IRC Network, Alex!alex@host");
```

Which sends this over the network:

```text
:irc.example.org 001 Alex :Welcome to the IRC Network, Alex!alex@host
```

---

## Commands to Implement

 Command: USER 
 `Parameters: <username> <hostname> <servername> <realname>`
 https://www.rfc-editor.org/rfc/rfc1459#section-4.1.3

 
 Command: JOIN
 `Parameters: <channel>{,<channel>} [<key>{,<key>}]`
 https://www.rfc-editor.org/rfc/rfc1459#section-4.2.1
 
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
 
 Command: LIST
 `Parameters: [<channel>{,<channel>} [<server>]]`
 https://www.rfc-editor.org/rfc/rfc1459#section-4.2.6

---

## bircd - Basic IRC Daemon

**`bircd`** is a lightweight IRC server implementation written in C. It serves as a learning tool for getting familiar with non-blocking I/O, TCP server setup, and basic message relaying. As such, `bircd` provides a solid starting point for implementing a fully functional IRC server.   
As a network daemon, `bircd` runs continuously in the background, listening for client connections and facilitating communication between them.

---

### ✅ What bircd covers (fundamentals):  

- Basic TCP server in C using `socket()`, `bind()`, `listen()`, `accept()`
- Handles multiple simultaneous client connections using `select()`
- Broadcasts messages between clients (basic real-time messaging)

---

### ❌ What bircd lacks for a complete IRC server:
| Requirement                        | bircd                                         | Full IRC Implementation       |
|------------------------------------|-----------------------------------------------|-------------------------------|
| Written in C++98                  | ❌ C only                                     | ✅ Yes                        |
| Command parsing (NICK, USER, etc) | ❌ Not implemented                            | ✅ Yes                        |
| IRC protocol compliance           | ❌ Not implemented                            | ✅ Yes                        |
| Channel support (#channel)        | ❌ Not implemented                            | ✅ Yes                        |
| Private messaging (/msg)          | ❌ Not implemented                            | ✅ Yes                        |
| Operator commands (KICK, INVITE…) | ❌ Not implemented                            | ✅ Yes                        |
| Password on connection            | ❌ Not implemented                            | ✅ Yes                        |
| Poll()/select() limit: only one   | ⚠️ Possibly compliant                         | ✅ Must be enforced           |
| Non-blocking sockets              | ✅ Mostly OK                                  | ✅ Strictly required          |
| Handling partial messages         | ❌ Not implemented (write buffer unused)      | ✅ Yes (testable via `nc`)    |

---

### Key Files

- `bircd.h` – Main header with structures and function declarations
- `main.c` – Entry point and setup
- `srv_create.c` – TCP server socket creation and binding
- `srv_accept.c` – Handles new client connections
- `client_read.c` – Message receiving and relaying logic
- `client_write.c` – Placeholder for message sending logic
- `main_loop.c` – Core event loop with `select()`

---

### How It Works

#### 1. Server Initialization
```c
t_env e;
init_env(&e);             // Initialize environment
get_opt(&e, ac, av);      // Parse command line arguments
srv_create(&e, e.port);   // Create and bind TCP socket
main_loop(&e);            // Enter main event loop
```

#### 2. Connection Handling
- Server listens on specified port
- New connections trigger `srv_accept()`
- Each client gets a unique file descriptor
- Client connections are tracked in the `fds` array

#### 3. Message Broadcasting
When a client sends a message:
1. `client_read()` receives the message
2. Message is stored in client's read buffer
3. Server iterates through all connected clients
4. Message is sent to every client except the sender
5. If send fails, client connection is cleaned up

---

### Usage

#### Building
```bash
cd bircd/
make
```

#### Running the Server
```bash
./bircd <port>

# Example:
./bircd 6667    # Start server on IRC standard port
```

**Note:** While you'll get an error like `bind error (srv_create.c, 36): Permission denied` if you try to use a port that's already in use or restricted, you can check in advance which ports are in use by running `netstat -tuln`.

---

#### Testing the Chat Server

##### Using netcat (nc)
```bash
# Terminal 1 (Client 1)
nc localhost 6667

# Terminal 2 (Client 2)
nc localhost 6667
```

##### Using telnet
```bash
# Terminal 1 (Client 1)
telnet localhost 6667

# Terminal 2 (Client 2)
telnet localhost 6667
```

Then:
- Type messages from one terminal → see them appear in the other
- Use `Ctrl+C` or close the terminal to disconnect

---

## References

<a name="footnote1">[1]</a> Oikarinen, J.; Reed, D.(1993). *Internet Relay Chat Protocol*. [Request for Comments: 1459](https://www.rfc-editor.org/rfc/rfc1459)
