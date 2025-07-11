# bircd - Basic IRC Daemon

## Overview

**bircd** is a lightweight IRC (Internet Relay Chat) server implementation written in C. It provides the fundamental networking infrastructure for a simple chat server, supporting multiple simultaneous client connections and real-time message broadcasting.

## Features

- **Multi-client Support**: Handles multiple simultaneous connections using `select()` I/O multiplexing
- **Real-time Broadcasting**: Messages from any client are instantly broadcast to all other connected clients
- **TCP Socket Server**: Robust TCP server implementation with proper connection handling
- **Event-driven Architecture**: Efficient event loop using `select()` system call
- **Memory Management**: Clean file descriptor management and resource cleanup

## Architecture

### Core Components

1. **File Descriptor Management** (`t_fd` structure):
   - Tracks connection types (server, client, free)
   - Manages read/write function pointers
   - Buffers for incoming and outgoing data

2. **Environment Structure** (`t_env`):
   - Central state management
   - File descriptor sets for `select()`
   - Port configuration and connection tracking

3. **Event Loop**:
   - Main server loop using `select()`
   - Handles new connections, client reads/writes
   - Manages connection cleanup

### Key Files

- `bircd.h` - Main header with structures and function declarations
- `main.c` - Entry point and initialization
- `srv_create.c` - TCP server socket creation and binding
- `srv_accept.c` - New client connection handling
- `client_read.c` - Message receiving and broadcasting logic
- `client_write.c` - Message sending functionality
- `main_loop.c` - Core event loop with `select()`

## How It Works

### 1. Server Initialization
```c
t_env e;
init_env(&e);                    // Initialize environment
get_opt(&e, ac, av);            // Parse command line arguments
srv_create(&e, e.port);         // Create and bind TCP socket
main_loop(&e);                  // Enter main event loop
```

### 2. Connection Handling
- Server listens on specified port
- New connections trigger `srv_accept()`
- Each client gets a unique file descriptor
- Client connections are tracked in the `fds` array

### 3. Message Broadcasting
When a client sends a message:
1. `client_read()` receives the message
2. Message is stored in client's read buffer
3. Server iterates through all connected clients
4. Message is sent to every client except the sender
5. If send fails, client connection is cleaned up

### 4. Event Loop
```c
// Simplified main loop logic
while (1) {
    do_select(&e);              // Wait for I/O events
    check_fd(&e);               // Process ready file descriptors
}
```

## Usage

### Building
```bash
cd bircd/
make
```

### Running the Server
```bash
./bircd <port>
```

**Example:**
```bash
./bircd 6667    # Start server on IRC standard port
```

### Connecting Clients

#### Using telnet
```bash
# Terminal 1 (Client 1)
telnet localhost 6667

# Terminal 2 (Client 2)
telnet localhost 6667
```

#### Using netcat (nc)
```bash
# Terminal 1
nc localhost 6667

# Terminal 2
nc localhost 6667
```

### Testing the Chat
1. Connect multiple clients to the server
2. Type messages in any client terminal
3. Messages appear in all other connected clients
4. Use Ctrl+C or close terminal to disconnect

### Gracefully Closing the Server

Currently, bircd doesn't have built-in graceful shutdown handling. Here are the methods to stop the server:

#### Method 1: Keyboard Interrupt (Recommended)
```bash
# In the terminal running bircd
Ctrl+C
```
This sends SIGINT to the process, causing it to terminate immediately.

#### Method 2: Kill Process
```bash
# Find the process ID
ps aux | grep bircd

# Kill gracefully with SIGTERM
kill <PID>

# Force kill if needed
kill -9 <PID>
```

#### Method 3: Using pkill
```bash
pkill bircd
```

#### Checking if Server is Stopped
```bash
# Check if any process is listening on your port
netstat -tuln | grep 6667
# or
ss -tuln | grep 6667

# Should return nothing if server is stopped
```

## Example Session

```bash
# Terminal 1: Start server
$ ./bircd 6667

# Terminal 2: Client 1
$ telnet localhost 6667
Hello everyone!

# Terminal 3: Client 2  
$ telnet localhost 6667
Hello everyone!          # <- Message from Client 1 appears here
Hi back!

# Terminal 2: Client 1
Hi back!                 # <- Message from Client 2 appears here
```

## Server Output
The server displays connection information:
```
New client #4 from 127.0.0.1:46374
New client #5 from 127.0.0.1:38224
client #4 gone away
```

## Port Recommendations

- **6667**: Standard IRC port (recommended)
- **6668-6670**: Alternative IRC ports
- **8000-9999**: Safe development ports

## Technical Details

### File Descriptor Types
- `FD_FREE` (0): Available file descriptor slot
- `FD_SERV` (1): Server listening socket
- `FD_CLIENT` (2): Connected client socket

### Buffer Management
- Read/write buffers: 4096 bytes each
- Messages larger than buffer size are truncated
- Buffers are cleaned on client disconnect

### Error Handling
- Uses custom error handling macros (`X` and `Xv`)
- Graceful handling of client disconnections
- Resource cleanup on errors

## Write Functionality - The Missing Piece

### Current Implementation Status

bircd implements a **partial write system** that demonstrates good architecture but lacks actual functionality:

```c
// In srv_accept.c - Function pointer is assigned
e->fds[cs].fct_write = client_write;  ✅ Polymorphic setup complete

// In init_fd.c - Buffer check (always false)
if (strlen(e->fds[i].buf_write) > 0)  ❌ Empty buffer condition
    FD_SET(i, &e->fd_write);          ❌ Never adds to write set

// In check_fd.c - Call exists but never executes
if (FD_ISSET(i, &e->fd_write))        ❌ Not in write set
    e->fds[i].fct_write(e, i);        ❌ Dead code

// In client_write.c - Empty implementation
void client_write(t_env *e, int cs)   ✅ Function exists
{                                     ❌ Does nothing
}
```

### Why buf_write is Unused

The `buf_write` buffer in each `t_fd` structure is **never written to** because:

1. **No message queuing** - Messages are sent directly in `client_read()`
2. **No server-generated messages** - No MOTD, user lists, or system notifications
3. **No buffering logic** - No code to accumulate outgoing data
4. **Immediate broadcasting** - All communication happens synchronously

### Testing the Limitation

The missing write system becomes apparent when testing with partial data:

```bash
# This test reveals the fragmentation issue:
nc -C 127.0.0.1 6667
com^D     # Sends "com" → broadcasts "com"
man^D     # Sends "man" → broadcasts "man"  
d         # Sends "d\n" → broadcasts "d\n"
# Result: 3 fragmented messages instead of 1 complete "command"
```

## Limitations

- **No IRC Protocol**: Basic message relay, no IRC commands
- **No Authentication**: No user login or nicknames
- **No Channels**: Single global chat room
- **No Message History**: No persistence or logging
- **Buffer Limits**: 4KB message size limit

## Extending bircd

To add IRC protocol features:
1. Parse IRC commands in `client_read()`
2. Implement command handlers (JOIN, NICK, etc.)
3. Add channel management structures
4. Implement user authentication

### Adding Graceful Shutdown
The current implementation lacks graceful shutdown. To improve this:

1. **Add Signal Handling**:
```c
#include <signal.h>

volatile sig_atomic_t running = 1;

void signal_handler(int sig) {
    running = 0;
}

int main(int ac, char **av) {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    // ... rest of main
}
```

2. **Modify Main Loop**:
```c
void main_loop(t_env *e) {
    while (running) {  // Instead of while(1)
        init_fd(e);
        do_select(e);
        check_fd(e);
    }
    cleanup_server(e);  // Add cleanup function
}
```

3. **Add Cleanup Function**:
```c
void cleanup_server(t_env *e) {
    int i;
    
    // Close all client connections
    for (i = 0; i <= e->maxfd; i++) {
        if (e->fds[i].type != FD_FREE) {
            close(i);
        }
    }
    
    // Free allocated memory
    free(e->fds);
    
    printf("Server shutdown complete.\n");
}
```
