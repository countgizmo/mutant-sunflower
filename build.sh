#!/bin/bash

rm -r build
mkdir -p build

# Using sdl2-config
# doesn't work probably because of the TTF extension
# gcc -o ./build/mutant_sunflower $(sdl2-config --cflags --libs)

# This variant requires brew install of the library first
gcc -o ./build/mutant_sunflower -lSDL2 -lSDL2_ttf \
-I/Library/Frameworks/SDL2_ttf.framework/Headers \
-I/Library/Frameworks/SDL2.framework/Headers ./code/main.c

# This variant requires frameworks to be available (SDL2's README for macs says to just copy the framework directory)
# gcc -o ./build/mutant_sunflower \
#   -I/Library/Frameworks/SDL2.framework/Headers \
#   -I/Library/Frameworks/SDL2_ttf.framework/Headers \
#   -F/Library/Frameworks/SDL2.framework \
#   -g ./code/main.c
