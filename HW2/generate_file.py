from sys import argv
from random import random

num = 16

if len(argv) > 2:
	num = int(argv[2])

fout = open(argv[1], 'w')
for i in range(num):
	fout.write('%d\n' % (int(random() * 1024)))

fout.close()
