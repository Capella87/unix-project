cmake_minimum_required(VERSION 3.10)
project(unix-project C)

set(basic CMAKE_C_STANDARD 11)

set(CMAKE_BUILD_TYPE Debug)

add_executable(basic_client "src/basic/client.c")
add_executable(basic_server "src/basic/server.c")
add_executable(server_server "src/server-oriented/server.c")
add_executable(server_client "src/server-oriented/client.c")
add_executable(client_server "src/client-oriented/server.c")
add_executable(client_client "src/client-oriented/client.c")

add_executable(data_provider "src/data-sender.c")
add_executable(data_validator "src/data-validator.c")

add_executable(packet "src/packet.c")