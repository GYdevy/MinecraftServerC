# Minecraft Server in C
A lightweight Minecraft server implementation written in C, focused on low-level networking and protocol handling. Supports real-time player connection, world spawning, and movement processing. Designed for educational and experimental purposes.

## How to Use

```bash
docker build -t minecraftserverc .
docker run -it -p 61243:61243 --name minecraftserverc minecraftserverc
```

## Features
#### Implements core Minecraft server functionality:
- Accepts connections from Minecraft clients.
- Handles login, world initialization, and player spawning.
- Processes player movement packets and maintains session state.
#### Built entirely from scratch:
- No external game libraries; direct implementation of the Minecraft protocol
- Uses low-level TCP sockets (POSIX) for network communication
- Manual packet serialization and deserialization in compliance with the protocol specification

## TODO features
- Player chat messages are not yet implemented.
- Server doesn't handle player disconnections gracefully.
- World saving is not persistent.
- Multiple players can join but not see each other.



