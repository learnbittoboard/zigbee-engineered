# Chapter 7, Step 1 — The Occupancy Sensing endpoint.
# Structure follows the Zigbee add-on's "template" sample via the
# Chapter 6 skeleton: same chassis, new payload.
cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(ch07_occupancy_endpoint)

target_sources(app PRIVATE src/main.c)
target_include_directories(app PRIVATE include)
