#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
//#include "read_api.h"

typedef struct __attribute__((packed)) {
    uint8_t dir_name[11];          //Short name
    uint8_t dir_attr;              //ATTR_READ_ONLY    0x01
                                //ATTR_HIDDEN       0x02
                                //ATTR_SYSTEM       0x04
                                //ATTR_VOLUME_ID    0x08
                                //ATTR_DIRECTORY    0x10
                                //ATTR_ARCHIVE      0x20
                                //ATTR_LONG_NAME    0x0F
    uint8_t dir_NTRes;             //Reserved for Windows NT
    uint8_t dir_crtTimeTenth;      //Millsecond stamp at creation time
    uint16_t dir_crtTime;      //Time file was created
    uint16_t dir_crtDate;      //Date file was created
    uint16_t dir_lstAccDate;   //Last access date
    uint16_t dir_fstClusHI;    //High word of entry's first cluster number - 0 for FAT16
    uint16_t dir_wrtTime;      //Time of last write
    uint16_t dir_wrtDate;      //Date of last write
    uint16_t dir_fstClusLO;    //Low word of entry's first cluster numer
    uint32_t dir_fileSize;           //32 bit word holding size in bytes
} dirEnt;

int OS_cd(const char * path);
int OS_open(const char * path);
int OS_close(int fd);
int OS_read(int fildes, void * buf, int nbyte, int offset);
dirEnt * OS_readDir(const char * dirname);

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
	entries = OS_readDir(".");
    print_entries(entries);
	printf("\n");

    int count = 0;
    while (1)   {
        if (entries[count].dir_name[0] == 0)
            break;
        printf("main:\t%d\tEntry Name:\t%.11s\n", count, entries[count].dir_name);
        char * subname = entries[count].dir_name;
        char * padding = strchr(subname, 0x20);
        if (padding != NULL)
            subname[padding - subname] = '\0';
        printf("main:\t%s\t%lu\n", subname, strlen(subname));
        dirEnt* subentries = OS_readDir(subname);
        print_entries(subentries);
        free(subentries);
        printf("\n");
        count ++;
    }
    
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
    
    int fd2 = OS_open("DHO2B/THE-GAME.TXT");
    printf("main:\tFile Descriptor:\t%d\n\n", fd2);
    bread = OS_read(fd2, (void*)buff, nbytes, offset);
    printf("main:\tBytes Read:\t%d\n\n", bread);
    if (bread < nbytes)
        buff[bread] = '\0';
    printf("%s\n", buff);

    bread = OS_read(fd2, (void*)buff, nbytes, 4200);
    printf("main:\tBytes Read:\t%d\n\n", bread);
    if (bread < nbytes)
        buff[bread] = '\0';
    printf("%s\n", buff);

    OS_close(fd);
    OS_close(fd2);

    
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
    printf("main:\t%d\n", OS_cd(("/People/PRF4V/SOARING.TXT/../")));
    
    return 0;
}
