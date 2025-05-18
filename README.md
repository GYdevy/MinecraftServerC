# Minecraft Server in C
A lightweight Minecraft server implementation written in C, focused on low-level networking and protocol handling. Supports real-time player connection, world spawning, and movement processing. Designed for educational and experimental purposes.

##  Project Status

This project has reached the goals I originally set for it: implementing login, chunk generation, player movement, and multiplayer visibility. It demonstrates a scalable foundation within the constraints of the protocol and serves as a solid educational exploration of low-level networking and protocol handling.

While the core functionality is complete, additional features and production-grade components are outside the intended scope, as the project's primary purpose was learning and experimentation.


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
- Handles chat messages between players.
#### Built entirely from scratch:
- No external game libraries; direct implementation of the Minecraft protocol
- Uses low-level TCP sockets (POSIX) for network communication
- Manual packet serialization and deserialization in compliance with the protocol specification

## TODO features
- World saving is not persistent.
- Generate world.

## Special Thanks
![image](https://github.com/user-attachments/assets/4a5a0100-0764-4c75-adec-b61d230315a9)

## Contributing

Contributions are welcome and appreciated!

If you'd like to help improve this project:

1. Fork the repository.
2. Create a new branch: `git checkout -b feature-name`.
3. Make your changes and commit them.
4. Push to your fork: `git push origin feature-name`.
5. Open a pull request.

Please follow the existing coding style and include relevant documentation or tests if applicable.

---

## License

This project is licensed under the [MIT License](LICENSE).

You are free to use, modify, and distribute this software with proper attribution. See the `LICENSE` file for details.


