cmake_minimum_required(VERSION 3.17)
project(siglog C)

set(CMAKE_C_STANDARD 99)

add_executable(siglog test.c siglog.c siglog.h)