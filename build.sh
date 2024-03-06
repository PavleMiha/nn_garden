#!/bin/bash

# Check if the build directory exists, if not create it
if [ ! -d "./build" ]; then
    mkdir ./build
fi

# Check if genie exists, if not download it
if [ ! -f "./build/genie" ]; then
    echo "Downloading genie..."
    curl -L https://github.com/bkaradzic/bx/raw/master/tools/bin/darwin/genie -o ./build/genie
    chmod +x ./build/genie
fi

# Run genie with the provided parameter
./build/genie "$@"
