# Chapter 9, Step 2 — The bad v2: a perfectly signed update that
# never confirms its image (and reboots to prove what that costs).
cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(ch09_rollback_bad)

target_sources(app PRIVATE src/main.c src/ld2410.c)
target_include_directories(app PRIVATE include)
