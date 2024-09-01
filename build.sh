#!/bin/bash

# Check if a filename was provided
if [ $# -eq 0 ]; then
    echo "Please provide a filename (without .cpp extension) as an argument."
    exit 1
fi

name=$1

# Check if tigr.c exists, if not download it
if [ ! -f tigr.c ]; then
    wget https://raw.githubusercontent.com/benonymity/CMP-201/main/homework/Assignment%205/tigr.c -q -O tigr.c
fi

# Check if tigr.h exists, if not download it
if [ ! -f tigr.h ]; then
    wget https://raw.githubusercontent.com/benonymity/CMP-201/main/homework/Assignment%205/tigr.h -q -O tigr.h
fi

# # Check if Metal.hpp exists, if not download it
# if [ ! -f Metal.hpp ]; then
#     wget https://raw.githubusercontent.com/benonymity/CMP-201/main/homework/Assignment%205/Metal.hpp -q -O Metal.hpp
# fi


if [[ "$OSTYPE" == "darwin"* ]]; then
    # Check if an existing boids process is running and kill it if found
    if pgrep -x "boids" > /dev/null; then
        pkill -x "boids"
        echo "Killed existing boids process"
    fi

    g++ -std=c++17 ${name}.cpp tigr.c -o ${name} -framework OpenGL -framework Cocoa # I'll need this once I'm offically using Metal: -framework Foundation -framework Metal -framework MetalKit
    ./${name} &
elif [[ "$OSTYPE" == "msys"* ]] || [[ "$OSTYPE" == "cygwin"* ]] || [[ "$OSTYPE" == "win"* ]]; then
    # Windows-specific compilation
    g++ -std=c++17 ${name}.cpp tigr.c -o ${name} -s -lopengl32 -lgdi32
    ./${name}
else
    # For other operating systems, attempt a generic compilation
    g++ -std=c++17 ${name}.cpp tigr.c -o ${name}
    ./${name}
fi

