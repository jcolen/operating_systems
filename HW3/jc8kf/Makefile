default:
	gcc -o libFAT.o -c -fpic read_api.c
	gcc -shared -o libFAT16.so libFAT.o 
	cp libFAT16.so libFAT32.so
	rm *.o

clean: 
	rm libFAT**.so	
