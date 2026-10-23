# Chapter 3, Step 3 — One button, four modes: short and long presses.
cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(ch03_button_modes)

target_sources(app PRIVATE src/main.c)
