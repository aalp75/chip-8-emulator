#!/bin/bash
set -e

g++ -std=c++23 main.cpp -o main.out $(sdl2-config --cflags --libs)
./main.out