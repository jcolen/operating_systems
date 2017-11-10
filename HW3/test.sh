#!/bin/bash

make

export LD_LIBRARY_PATH=/home/jcolen19/cs4414/HW3:$$LD_LIBRARY_PATH
export FAT_FS_PATH=/home/jcolen19/cs4414/HW3/sampledisk16.raw
gcc test.c -L/home/jcolen19/cs4414/HW3 -lFAT16
./a.out > output_16.txt

export FAT_FS_PATH=/home/jcolen19/cs4414/HW3/sampledisk32.raw
gcc test.c -L/home/jcolen19/cs4414/HW3 -lFAT32
./a.out > output_32.txt

diff output_16.txt output_32.txt
