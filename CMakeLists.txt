cmake_minimum_required(VERSION 3.30)
project(MinecraftServerC)

set(CMAKE_C_STANDARD 23)


# Add your executable target
add_executable(server
        main.c
        socket_utils.c
        packet_utils.c
        packet_utils.h
        handshake.c
        handshake.h
        socket_utils.h
        login.c
        login.h
        Externals/cJSON.c
        Externals/cJSON.h
        Externals/libnbt/libnbt/miniz.c
        Externals/libnbt/libnbt/miniz.H
        nbt_impl.c
        extract_uuid.c
        extract_uuid.h
        play.c
        play.h
)

# Specify the location of libnbt include directory
include_directories(F:/MinecraftserverC/Externals/libnbt/libnbt)

# Link to the libnbt library (adjust this if it's a static or dynamic library)
target_link_libraries(server ws2_32)

# Find and link curl from vcpkg correctly
find_package(CURL REQUIRED)
target_link_libraries(server CURL::libcurl)
