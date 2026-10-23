# Chapter 2, Step 4 — Work queues defer interrupt work to a thread.
cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(ch02_workqueue)

target_sources(app PRIVATE src/main.c)
