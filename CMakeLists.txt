cmake_minimum_required(VERSION 3.25)
project(vjudge_cli C)

set(CMAKE_C_STANDARD 11)

add_executable(vjudge_cli main.c libvcd.h)
