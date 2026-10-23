# Chapter 2, Step 3 — Kconfig options and per-module log levels.
cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(ch02_kconfig)

target_sources(app PRIVATE src/main.c src/sensor_sim.c)
