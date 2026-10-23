# Chapter 3, Step 1 — Every edge the button makes, counted raw.
cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(ch03_meet_the_bounce)

target_sources(app PRIVATE src/main.c)
