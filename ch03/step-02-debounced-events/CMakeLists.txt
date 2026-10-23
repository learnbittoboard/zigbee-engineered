# Chapter 3, Step 2 — Timestamp debounce: one press, one event.
cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(ch03_debounced_events)

target_sources(app PRIVATE src/main.c)
