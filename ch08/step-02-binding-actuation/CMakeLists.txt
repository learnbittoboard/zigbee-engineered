# Chapter 8, Step 2 — Binding & actuation: occupancy switches the bulb.
cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(ch08_binding_actuation)

target_sources(app PRIVATE src/main.c src/ld2410.c)
target_include_directories(app PRIVATE include)
