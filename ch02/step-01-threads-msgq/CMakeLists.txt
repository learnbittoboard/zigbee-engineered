# Chapter 2, Step 1 — Threads and a message queue.
cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(ch02_threads_msgq)

target_sources(app PRIVATE src/main.c)
