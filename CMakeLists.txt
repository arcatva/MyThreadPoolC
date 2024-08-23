cmake_minimum_required(VERSION 3.29)
project(MyThreadPoolC C)

set(CMAKE_C_STANDARD 11)

add_executable(MyThreadPoolC main.c
        threadpool.c
        threadpool.h)
