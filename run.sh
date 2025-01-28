#!/bin/bash

bmp2code.exe -i gfx/gfx.bmp -bw -ci 1 0xFFFFFF -iw 8 -ih 8 -of ANSI_C -o gfx/gfx.h
bmp2code.exe -i gfx/sprites.bmp -dummy 1 -ci 0 0x00 -ci 1 0xFF0000 -ci 3 0x0000FF -ci 2 0xFFFFFF -iw 12 -ih 21 -od sprites -of ANSI_C -o gfx/sprites.h
bmp2code.exe -i gfx/sprites_bw.bmp -dummy 1 -ci 1 0xFFFFFF -iw 24 -ih 21 -od sprites_bw -of ANSI_C -o gfx/sprites_bw.h

make clean
make all
mv ggj.prg bin/ggj.prg 

if [ "$1" == "c64" ]; then
/Applications/VICE_3_8/bin/x64sc -model c64 -autostart "bin/ggj.prg"
fi
