# Chapter 6, Step 1 — Occupancy-sensor router skeleton.
# Structure follows the nRF Connect Zigbee add-on's "template" sample,
# which is Nordic's intended starting point for a custom router device.
cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(ch06_router_skeleton)

target_sources(app PRIVATE src/main.c)
target_include_directories(app PRIVATE include)
