#!/bin/bash

rm -rf ./main
g++ -o main $(find -name "*.cpp") -I./include

