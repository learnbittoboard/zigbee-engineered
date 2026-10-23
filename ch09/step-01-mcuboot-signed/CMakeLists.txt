# Chapter 9, Step 1 — The product, behind MCUboot: signed image,
# version stamp, and a cable-DFU door (mcumgr over the console UART).
cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(ch09_mcuboot_signed)

target_sources(app PRIVATE src/main.c src/ld2410.c)
target_include_directories(app PRIVATE include)
