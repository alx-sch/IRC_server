# IRC Server


This project is a collaboration between:

- **[Patrícia](https://github.com/paribeir)**: TASKS TDB
- **[Pavlos](https://github.com/sysex89)**: TASKS TDB
- **[Alex](https://github.com/alx-sch)**: TASKS TDB


## IRC Introduction

- What is it, history etc.
- ports to use etc.


## Networking Basics

- What is a socket?
- What is a port?
- TCP vs UDP
- What is a client/server model?
- I/O multiplexing (select, poll)
- Blocking vs non-blocking I/O


## bircd - Basic IRC Daemon

**`bircd`** is a lightweight IRC server implementation written in C. It serves as a learning tool for getting familiar with non-blocking I/O, TCP server setup, and basic message relaying. As such, `bircd` provides a solid starting point for implementing a fully functional IRC server.

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

