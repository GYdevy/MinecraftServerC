cmake_minimum_required(VERSION 3.16.3)
project(MinecraftServerC)

# Set the C standard to 11
set(CMAKE_C_STANDARD 11)

# Create the executable with absolute or relative paths to your source files
add_executable(server
    ${CMAKE_SOURCE_DIR}/main.c
    ${CMAKE_SOURCE_DIR}/socket_utils.c
    ${CMAKE_SOURCE_DIR}/packet_utils.c
    ${CMAKE_SOURCE_DIR}/handshake.c
    ${CMAKE_SOURCE_DIR}/login.c
    ${CMAKE_SOURCE_DIR}/extract_uuid.c
    ${CMAKE_SOURCE_DIR}/play.c
    ${CMAKE_SOURCE_DIR}/server.c
    ${CMAKE_SOURCE_DIR}/movement.c
    ${CMAKE_SOURCE_DIR}/play_helpers.c
    ${CMAKE_SOURCE_DIR}/chat.c
)

# Include Externals/libnbt directory for headers
target_include_directories(server PRIVATE ${CMAKE_SOURCE_DIR}/Externals/libnbt)

# Find and link CURL
find_package(CURL REQUIRED)
target_link_libraries(server PRIVATE CURL::libcurl)

# Link other libraries if necessary (e.g., libnbt)
# target_link_libraries(server PRIVATE nbt)
