# Chapter 1, Step 1 — Hello, logging.
# Every Zephyr application starts with these same three lines:
# find the Zephyr package, name the project, add sources.
cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(ch01_hello_logging)

target_sources(app PRIVATE src/main.c)
