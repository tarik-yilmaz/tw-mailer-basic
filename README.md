# TW-Mailer Basic (Client + Server)

Simple TCP client-server app in C++ for sending/receiving messages via a plain-text protocol.

- **Server**: Iterative TCP server stores messages in a spool directory.
- **Client**: CLI to connect and run commands.

## Features
Commands:
- `SEND`: Send message.
- `LIST`: List user inbox subjects.
- `READ`: Read specific message.
- `DEL`: Delete specific message.
- `QUIT`: Close connection.
- **Enhancements**:
    - `LIST` includes timestamp.
    - Client automatically lists messages before `READ`/`DEL` if needed.

## Requirements
- Linux/WSL
- C++17 compiler (e.g., g++)
- Make

## Structure
```
.
├── twmailer-client.cpp
├── twmailer-server.cpp
├── common.hpp
├── Makefile
├── project_report.md
└── README.md
```

## Build
```bash
make
```
Clean: `make clean`

## Run
Server: `./twmailer-server <port> <spool-dir>` (e.g., `./twmailer-server 8080 mailspool`)

Client: `./twmailer-client <ip> <port>` (e.g., `./twmailer-client 127.0.0.1 8080`)

In client: Enter SEND, LIST, READ, DEL, QUIT.

## Protocol
Newline-delimited. Messages end with `.` on own line.

**SEND**  
Client: `SEND\n<sender>\n<receiver>\n<subject>\n<body lines>\n.\n`  
Server: `OK\n` or `ERR\n`

**LIST**  
Client: `LIST\n<user>\n`  
Server: `<count>\n<subject1>\n...`

**READ**  
Client: `READ\n<user>\n<msg_num>\n`  
Server: `OK\n<message content>\n.\n` or `ERR\n`

**DEL**  
Client: `DEL\n<user>\n<msg_num>\n`  
Server: `OK\n` or `ERR\n`

**QUIT**  
Client: `QUIT\n` (no response)

## Validation
- Usernames: Max 8 chars, a-z0-9.
- Subject: Max 80 chars.

## Persistence
Messages stored in spool dir (e.g., per-user files or mbox). Survive restarts.

## Testing
Use client to SEND/LIST/READ/DEL. Or netcat for manual tests.

## Notes
- Iterative server (one client at a time).
- Focus: No leaks, robust I/O, error handling.

## Deliverables
- Source code
- Makefile (all/clean)
- 1-page PDF: Architecture, tech, strategy.

Authors: Tarik Yilmaz (Client), Peyman Aparviz (Server)