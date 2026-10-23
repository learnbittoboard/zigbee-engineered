# Chapter 1, Step 2 — A shell command of your own.
cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(ch01_shell_command)

target_sources(app PRIVATE src/main.c)
