#!/bin/bash

make

cp ../HW3/sampledisk16.raw .
cp ../HW3/sampledisk32.raw .

export LD_LIBRARY_PATH=/home/jcolen19/cs4414/HW4:$$LD_LIBRARY_PATH
export FAT_FS_PATH=/home/jcolen19/cs4414/HW4/sampledisk16.raw
gcc ../HW3/test.c -L/home/jcolen19/cs4414/HW4 -lFAT16
./a.out > output_16_read.txt
gcc test.c -L/home/jcolen19/cs4414/HW4 -lFAT16
./a.out > output_16_write.txt

export FAT_FS_PATH=/home/jcolen19/cs4414/HW4/sampledisk32.raw
gcc ../HW3/test.c -L/home/jcolen19/cs4414/HW4 -lFAT32
./a.out > output_32_read.txt
gcc test.c -L/home/jcolen19/cs4414/HW4 -lFAT32
./a.out > output_32_write.txt

echo "Write differences"
diff output_16_write.txt output_32_write.txt

echo "Read differences"
diff output_16_read.txt output_32_read.txt

echo "Past differences"
diff output_16_read.txt ../HW3/output_16.txt
diff output_32_read.txt ../HW3/output_32.txt
