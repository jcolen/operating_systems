#include <stdio.h>
#include <stdlib.h>
#include "read_api.h"

int main()  {
    printf("main:\tSize of DIRENTRY: %lu\n", sizeof(DIRENTRY));
    printf("main:\tSize of BPB_Structure: %lu\n", sizeof(BPB_Structure));
    char * base_dir = getenv("FAT_FS_PATH");
    printf("main:\tPath to FAT16 Volume: %s\n\n", base_dir);

	//int ret = OS_cd("/path1/path2/path3/path4");
	//printf("main:\t%d\n", ret);
	
	DIRENTRY * entries = OS_readDir("People/PRF4V");
	int i = 0;
	while (1)	{
		if (entries[i].DIR_Name[0] == 0)
			break;
		
		printf("main:\tEntry:\t%.11s\t%02x\t%d\t%d\t%d\n",
			entries[i].DIR_Name, 
			entries[i].DIR_Attr,
			entries[i].DIR_FstClusHI,
			entries[i].DIR_FstClusLO,
			entries[i].DIR_FileSize);
		i ++;
	}

	free(entries);

	printf("\n");

	int ret = OS_cd("People");
	printf("\nmain:\tCD Return Value:\t%d\n\n", ret);
	entries = OS_readDir("PRF4V");
	printf("\n");
	i = 0;
	while (1)	{
		if (entries[i].DIR_Name[0] == 0)
			break;
		
		printf("main:\tEntry:\t%.11s\t%02x\t%d\t%d\t%d\n",
			entries[i].DIR_Name, 
			entries[i].DIR_Attr,
			entries[i].DIR_FstClusHI,
			entries[i].DIR_FstClusLO,
			entries[i].DIR_FileSize);
		i ++;
	}

	free(entries);
	printf("\n");	

	entries = OS_readDir("/Media");

	free(entries);
	printf("\n");

	int fd = OS_open("PRF4V/SOARING.TXT");
	printf("\nmain:\tFile Descriptor:\t%d\n\n", fd);
	
	int nbytes = 10000;
	int offset = 0;
	char * buff = malloc(sizeof(char) * nbytes);
	int bread = OS_read(fd, buff, nbytes, offset);
	printf("%s\n", buff);
	printf("\nmain::\tBytes Read:\t%d\n\n", bread);

	fd = OS_close(fd);
	printf("\nmain:\tClose status:\t%d\n\n", fd);

    return 0;
}
