# Chapter 4, Step 3 — The parser becomes a driver: ld2410.c + ld2410.h.
cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(ch04_ld2410_driver)

target_sources(app PRIVATE src/main.c src/ld2410.c)
