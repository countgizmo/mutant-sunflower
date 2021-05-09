#!/bin/bash

rm -r build
mkdir -p build

# This variant requires brew install of the library first
#gcc -o ./build/mutant_sunflower -lSDL2 -lSDL2_ttf -g ./code/main.c

# This variant requires frameworks to be available (SDL2's README for macs says to just copy the framework directory)
gcc -o ./build/mutant_sunflower \
    -I/Library/Frameworks/SDL2.framework/Headers \
    -I/Library/Frameworks/SDL2_ttf.framework/Headers \
    -F/Library/Frameworks \
    -framework SDL2 \
    -framework SDL2_ttf \
    -g ./code/main.c