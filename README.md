# IRC Server

<p align="center">
    <img src="https://github.com/alx-sch/IRC_server/blob/main/.assets/ft_irc_badge.png" alt="ft_irc_badge.png" />
</p>


This project is a collaboration between:

- **[Patrícia](https://github.com/paribeir)**: TASKS TDB
- **[Pavlos](https://github.com/sysex89)**: TASKS TDB
- **[Alex](https://github.com/alx-sch)**: TASKS TDB

---

## IRC Introduction

- What is it, history etc.
- ports to use etc.

---

## Networking Basics

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
    - Range: `0`–`65535`
    - Common ports: `22` (SSH), `80` (HTTP), `6667` (IRC)

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
    - Allows a single-threaded server to monitor multiple sockets.
    - `select()` and `poll()` are syscalls to wait for activity on many fds:
        - Monitor client connections
        - Check which sockets are ready to read/write
    - Used to avoid spinning loops or threading.

---

## Core functions (WIP --> make sure to remove fcts not used)

| Function | Purpose | Declaration | Notes |
|----------|---------|-------------|-------|
| `socket()` | Create a socket (TCP communication) | `int socket(int domain, int type, int protocol);` | Typically: `AF_INET`, `SOCK_STREAM`, `0` |
| `bind()` | Assign IP and port to socket | `int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);` | Used for server sockets |
| `listen()` | Start listening for incoming connections | `int listen(int sockfd, int backlog);` | Turns socket into passive mode |
| `accept()` | Accept a new incoming connection | `int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);` | Returns a new FD for the client |
| `connect()` | Connect to a remote socket | `int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen);` | Used on the client side |
| `send()` | Send data to a peer | `ssize_t send(int sockfd, const void *buf, size_t len, int flags);` | Works on connected sockets |
| `recv()` | Receive data from a peer | `ssize_t recv(int sockfd, void *buf, size_t len, int flags);` | Works on connected sockets |
| `close()` | Close a socket or file descriptor | `int close(int fd);` | Frees system resources |
| `setsockopt()` | Set socket options | `int setsockopt(int sockfd, int level, int optname, const void *optval, socklen_t optlen);` | Commonly used with `SO_REUSEADDR` |
| `getsockname()` | Get local IP and port bound to socket | `int getsockname(int sockfd, struct sockaddr *addr, socklen_t *addrlen);` | Rarely used |
| `getprotobyname()` | Get protocol number (e.g. TCP) | `struct protoent *getprotobyname(const char *name);` | Deprecated in favor of hardcoded values |
| `gethostbyname()` | Resolve hostname to IP | `struct hostent *gethostbyname(const char *name);` | Deprecated; use `getaddrinfo()` |
| `getaddrinfo()` | Resolve host and port into socket structs | `int getaddrinfo(const char *node, const char *service, const struct addrinfo *hints, struct addrinfo **res);` | Preferred modern approach |
| `freeaddrinfo()` | Free memory from `getaddrinfo()` | `void freeaddrinfo(struct addrinfo *res);` | Must call after using `getaddrinfo()` |
| `htons()` / `htonl()` | Convert host to network byte order | `uint16_t htons(uint16_t hostshort);` / `uint32_t htonl(uint32_t hostlong);` | Used before sending ports/addresses |
| `ntohs()` / `ntohl()` | Convert network to host byte order | `uint16_t ntohs(uint16_t netshort);` / `uint32_t ntohl(uint32_t netlong);` | Used after receiving data |
| `inet_addr()` | Convert IPv4 string to binary | `in_addr_t inet_addr(const char *cp);` | Not thread-safe |
| `inet_ntoa()` | Convert binary IPv4 to string | `char *inet_ntoa(struct in_addr in);` | Not thread-safe |
| `signal()` | Basic signal handling | `void (*signal(int sig, void (*func)(int)))(int);` | Simple, less robust |
| `sigaction()` | Advanced signal handling | `int sigaction(int signum, const struct sigaction *act, struct sigaction *oldact);` | Recommended over `signal()` |
| `lseek()` | Move file descriptor position | `off_t lseek(int fd, off_t offset, int whence);` | Not typically needed for sockets |
| `fstat()` | Get metadata for FD | `int fstat(int fd, struct stat *statbuf);` | Used for type/size checks |
| `fcntl()` | Set non-blocking mode | `int fcntl(int fd, int cmd, ...);` | Only `F_SETFL, O_NONBLOCK` allowed in `ft_irc`; required on macOS, optional on Linux |
| `select()` | Monitor multiple FDs (I/O readiness) | `int select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout);` | Fine for small-scale use; limit = `FD_SETSIZE` |
| `poll()` | Modern I/O multiplexing | `int poll(struct pollfd fds[], nfds_t nfds, int timeout);` | Scales better with many clients; preferred for dynamic FD sets |




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

### Commands to Implement

 Command: USER 
 Parameters: <username> <hostname> <servername> <realname>
 https://www.rfc-editor.org/rfc/rfc1459#section-4.1.3

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

