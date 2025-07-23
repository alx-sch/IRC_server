# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build System

This project uses `make` with two separate build systems:

### Main C++ IRC Server (Root Directory)
```bash
# Build the main IRC server
make

# Clean object files
make clean

# Clean all build artifacts
make fclean

# Rebuild everything
make re
```
- Builds `ircserv` executable
- Uses C++98 standard with strict compiler flags (-Werror -Wextra -Wall -Wshadow -Wpedantic)
- Advanced Makefile with progress bars and dependency tracking
- 11 source files in `src/`, headers in `include/`, objects in `obj/`

### bircd Reference Implementation (bircd/ Directory)
```bash
# Build the reference C implementation
cd bircd/
make

# Clean
make clean && make fclean && make re
```
- Builds `bircd` executable (C implementation)
- Used as reference for understanding IRC server fundamentals

## Code Architecture

### Current Structure
The project is a substantial IRC server implementation in C++98 with ~1,600 lines of code across 11 source files:

#### Main Components (C++)
- **Server.hpp/cpp**: Core server class with complete TCP socket handling
  - `_usersFd`: Maps file descriptors to User objects
  - `_usersNick`: Maps nicknames to User objects  
  - `_channels`: Maps channel names to Channel objects
  - Uses `select()` for I/O multiplexing with non-blocking sockets
  - Signal handling for graceful shutdown (SIGINT/Ctrl+C)
  - Server metadata: "42ircRebels" v"eval-42.42"

- **User.hpp/cpp**: Complete user management system
  - Registration state tracking (`_hasNick`, `_hasUser`, `_hasPassed`, `_isRegistered`)
  - Input buffer management for partial message handling
  - Operator privileges tracking
  - IRC reply system for client communication

- **Channel.hpp/cpp**: Full channel system implementation
  - Channel modes: invite-only, topic protection, user limits, password protection
  - Member and operator management via `std::set<std::string>`
  - Invitation system and topic management

- **Command.hpp/cpp**: Robust command parsing and execution framework
  - Command tokenization and validation
  - Dispatch system for IRC command handlers

- **main.cpp**: Entry point requiring `<port> <password>` arguments

#### Reference Implementation (C)
- **bircd/**: Complete working IRC daemon in C
  - Uses `select()` for I/O multiplexing
  - Handles multiple clients with message broadcasting
  - Key files: `main.c`, `srv_create.c`, `srv_accept.c`, `client_read.c`, `main_loop.c`

### Development Status
- Currently on branch `exploration_pavlos`
- **Implementation Status**: Substantial working implementation (no longer "TBD")
- **Current Issue**: Build fails due to linker errors for `handleTopic` and `handleInvite` functions
- Active collaborative development with multiple contributors

## IRC Protocol Implementation

### Implemented Commands
- **NICK**: Full nickname validation and conflict handling
- **USER**: User registration with 4-parameter validation  
- **PASS**: Password authentication
- **JOIN**: Channel joining with automatic channel creation
- **PRIVMSG**: Private messaging to users and channels
- **TOPIC**: Function declared (has linker issues)
- **INVITE**: Function declared (has linker issues)

### Missing Commands (Need Implementation)
- **PART**: Channel leaving
- **QUIT**: Client disconnection
- **PING/PONG**: Keep-alive mechanism
- **KICK**: Remove users from channels
- **MODE**: Channel and user mode management
- **LIST**: Channel listing
- **OPER**: Operator privileges

### Protocol Compliance
- Basic IRC message parsing implemented
- Channel management with modes and operators
- User registration flow complete
- Missing comprehensive IRC numeric reply system
- Needs proper RFC 1459 error handling

## Testing

### bircd Testing
```bash
cd bircd/
./bircd 6667

# In separate terminals:
nc localhost 6667
telnet localhost 6667
```

### Main Server Testing
```bash
./ircserv 6667 password
```

## Current Issues

### Build Issues
- **Linker errors**: `handleTopic` and `handleInvite` functions declared but not implemented
- Build must be fixed before testing server functionality

### Development Priorities
1. Fix linker errors to enable building
2. Implement missing IRC commands (PART, QUIT, PING/PONG, KICK, MODE, LIST)
3. Add comprehensive error handling and IRC numeric replies
4. Complete RFC 1459 protocol compliance

## Key Implementation Notes

- Must use C++98 standard only
- Non-blocking sockets are required
- Only one of `select()` or `poll()` allowed for I/O multiplexing (using `select()`)
- Handle partial message reading/writing (testable with `nc`)
- Follow existing code style and structure patterns
- Color output macros available in `defines.hpp` (RED, GREEN, YELLOW, BOLD, RESET)
- Server uses dual mapping system for efficient user lookups by fd and nickname

## Project Context

This is a collaborative 42 School project implementing an IRC server. The README.md contains comprehensive networking fundamentals and IRC protocol documentation. The bircd reference implementation demonstrates core concepts like TCP server setup, `select()` usage, and basic message relaying. The C++ implementation has evolved into a sophisticated multi-class architecture with substantial functionality, requiring primarily bug fixes and missing command implementations to achieve full IRC server compliance.