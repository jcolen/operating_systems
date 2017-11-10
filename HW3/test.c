#include <stdio.h>
#include <stdlib.h>
#include "read_api.h"

void print_entries(dirEnt * entries)    {
	int i = 0;
	while (1)	{
		if (entries[i].dir_name[0] == 0)
			break;
		
		printf("print_entries:\tEntry:\t%.11s\t%02x\t%d\n",
			entries[i].dir_name, 
			entries[i].dir_attr,
			entries[i].dir_fileSize);
		i ++;
	}
}

int main()  {
    printf("main:\tSize of dirEnt: %lu\n", sizeof(dirEnt));
    char * base_dir = getenv("FAT_FS_PATH");
    printf("main:\tPath to FAT16 Volume: %s\n\n", base_dir);

	//int ret = OS_cd("/path1/path2/path3/path4");
	//printf("main:\t%d\n", ret);
	
	dirEnt * entries = OS_readDir("People/PRF4V");
    print_entries(entries);
	free(entries);

	printf("\n");

	int ret = OS_cd("People");
	printf("\nmain:\tCD Return Value:\t%d\n\n", ret);
	entries = OS_readDir("PRF4V");
    print_entries(entries);
	printf("\n");

	free(entries);
	printf("\n");	

	entries = OS_readDir("/Media");
    print_entries(entries);
	free(entries);
	printf("\n");

	int fd = OS_open("PRF4V/SOARING.TXT");
	printf("\nmain:\tFile Descriptor:\t%d\n\n", fd);
	
	int nbytes = 10000;
	int offset = 0;
	char * buff = malloc(sizeof(char) * nbytes);
	int bread = OS_read(fd, buff, nbytes, offset);
    if(bread < nbytes)
        buff[bread] = '\0';
	printf("%s\n", buff);
	printf("\nmain:\tBytes Read:\t%d\n\n", bread);

	fd = OS_close(fd);
	printf("\nmain:\tClose status:\t%d\n\n", fd);

    ret = OS_cd("/People");
    entries = OS_readDir("./");
    print_entries(entries);
    fd = OS_open("../Congrats.txt");
    printf("main:\tFile Descriptor:\t%d\n\n", fd);
    bread = OS_read(fd, buff, nbytes, offset);
    printf("main:\tBytes Read:\t%d\n\n", bread);
    if (bread < 0)
        bread = 0;
    if (bread < nbytes)  
        buff[bread] = '\0';
     
    printf("%s\n", buff);
    OS_close(fd);
    
    printf("main:\t%d\n", OS_cd("asdfdsaf"));
    printf("main:\t%d\n", OS_cd("/asdfdsafd"));
    printf("main:\t%d\n", OS_cd("/People/asdfsafsd"));
    printf("main:\t%d\n", OS_cd("PRF4V/SOARING.TXT"));
    printf("main:\t%d\n", OS_cd("././../People/PRF4V/../"));
    entries = OS_readDir("./");
    printf("main:\t%d\n", OS_open("./PRF4V/../asdfasfasf"));
    printf("main:\t%d\n", OS_close(-1));
    printf("main:\t%d\n", OS_close(40));
    printf("main:\t%d\n", OS_read(40, buff, 10000, 100));
    
    return 0;
}
