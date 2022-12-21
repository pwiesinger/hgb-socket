cmake_minimum_required(VERSION 3.23)
project(hgb_socket C)

set(CMAKE_C_STANDARD 23)

add_executable(hgb_socket main.c WordCheck.c)
