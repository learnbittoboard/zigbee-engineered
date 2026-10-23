# Chapter 2, Step 2 — Devicetree names for real hardware.
cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(ch02_devicetree)

target_sources(app PRIVATE src/main.c)
