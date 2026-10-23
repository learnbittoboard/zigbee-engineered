# Chapter 4, Step 2 — A frame-parser state machine for the LD2410.
cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(ch04_frame_parser)

target_sources(app PRIVATE src/main.c)
