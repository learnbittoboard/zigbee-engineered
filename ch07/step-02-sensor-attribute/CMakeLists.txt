# Chapter 7, Step 2 — The endpoint meets the sensor: LD2410 included.
cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(ch07_sensor_attribute)

target_sources(app PRIVATE src/main.c src/ld2410.c)
target_include_directories(app PRIVATE include)
