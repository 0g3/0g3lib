cmake_minimum_required(VERSION 3.12)
project(0g3lib C)

set(CMAKE_C_STANDARD 90)

add_executable(0g3lib main.c slice.h slice.c)