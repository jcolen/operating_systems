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

int OS_mkdir(const char * path);
int OS_rmdir(const char * path);
int OS_rm(const char * path);
int OS_creat(const char * path);
int OS_write(int fildes, const void * buf, int nbytes, int offset);

void print_entries(dirEnt * entries)    {
    if (entries == NULL)    {
        printf("print_entries:\tERROR:\tEntries is null\n\n");
        return;
    }
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
    printf("\n");
}

int main()  {
    printf("main:\tSize of dirEnt: %lu\n", sizeof(dirEnt));
    char * base_dir = getenv("FAT_FS_PATH");
    printf("main:\tPath to FAT16 Volume: %s\n\n", base_dir);

	//int ret = OS_cd("/path1/path2/path3/path4");
	//printf("main:\t%d\n", ret);
	
	//dirEnt * entries = OS_readDir("People/PRF4V");
    //print_entries(entries);
	//free(entries);

    dirEnt * entries = OS_readDir("/");
    print_entries(entries);
    free(entries);
    entries = OS_readDir("/");
    print_entries(entries);
    free(entries);

    int nbytes = 5000;
    int offset = 0;
    char buff[nbytes];

    int fd = OS_open("/Congrats.txt");
    printf("main:\tFile Descriptor:\t%d\n\n", fd);
    int bread = OS_read(fd, buff, nbytes, offset);
    printf("main:\tBytes Read:\t%d\n\n", bread);
    if (bread < 0)
        bread = 0;
    if (bread < nbytes)  
        buff[bread] = '\0';
     
    printf("%s\n", buff);
    
    const char * write_buff = "This is a a new test sentence";
    int bwrit = OS_write(fd, write_buff, strlen(write_buff), bread);
    printf("main:\tBytes Written:\t%d\n", bwrit);

    bread = OS_read(fd, buff, nbytes, offset);
    printf("main:\tBytes Read:\t%d\n\n", bread);
    if (bread < 0)
        bread = 0;
    if (bread < nbytes)
        buff[bread] = '\0';
    printf("%s\n", buff);

    
    int fd2 = OS_open("/People/DHO2B/THE-GAME.TXT");
    printf("main:\tFile Descriptor:\t%d\n\n", fd2);
    bread = OS_read(fd2, (void*)buff, nbytes, offset);
    printf("main:\tBytes Read:\t%d\n\n", bread);
    if (bread < nbytes)
        buff[bread] = '\0';
    printf("%s\n", buff);
   
    printf("main:\tBytes Written:\t%d\n", OS_write(fd2, buff, strlen(buff), bread));
    bread = OS_read(fd2, (void*)buff, nbytes + strlen(write_buff), 4000);
    if (bread < nbytes)
        buff[bread] = '\0';
    
    printf("main:\t%d\t%lu\n\n%s\n", bread, strlen(buff), buff);

    OS_close(fd);
    OS_close(fd2);

    entries = OS_readDir("/");
    print_entries(entries);
    free(entries);
   
    entries = OS_readDir("/People/DHO2B/");
    print_entries(entries);
    free(entries);

    printf("main:\t%d\n", OS_creat("/Media/../TEST.TXT"));          //1     file is created
    printf("main:\t%d\n", OS_creat("/Media/../abcde/TEST.TXT"));    //-1    path is invalid
    printf("main:\t%d\n", OS_creat("/Media/../Congrats.txt"));      //-2    already exists
    
    entries = OS_readDir("/");
    print_entries(entries);
    free(entries);

    fd = OS_open("/TEST.TXT");
    bwrit = OS_write(fd, write_buff, strlen(write_buff), 0);
    bread = OS_read(fd, buff, nbytes, 0);
    buff[bread] = '\0';
    printf("main:\tBytes Read:\t%d\n%s\n", bread, buff);
    OS_close(fd);
    
    entries = OS_readDir("/");
    print_entries(entries);
    free(entries);

    printf("main:\t%d\n", OS_mkdir("/NEWDIR"));                     //1     dir is created
    printf("main:\t%d\n", OS_mkdir("/asdfas/NEWDIR"));              //-1    path is invalid
    printf("main:\t%d\n", OS_mkdir("/Media"));                      //-2    already exists
    
    entries = OS_readDir("/");
    print_entries(entries);
    free(entries);

    entries = OS_readDir("/NEWDIR/");
    print_entries(entries);
    free(entries);

    entries = OS_readDir("/NEWDIR/././../Media/../");
    print_entries(entries);
    free(entries);

    printf("main:\t%d\n", OS_rm("/Media/.."));                      //-2    is a directory
    printf("main:\t%d\n", OS_rm("/Media/../TEST.TXT"));             //1
    printf("main:\t%d\n", OS_rm("/Media"));                         //-2    is a directory
    printf("main:\t%d\n", OS_rm("/NEWDIR/asdfas"));                 //-1    Path is invalid

    entries = OS_readDir("/");
    print_entries(entries);
    free(entries);
    
    printf("main:\t%d\n", OS_rmdir("/Media/../"));                   //-3    not empty
    printf("main:\t%d\n", OS_rmdir("/Media/../Congrats.txt"));      //-2    is a file
    printf("main:\t%d\n", OS_rmdir("/Media/"));                      //-3    not empty
    printf("main:\t%d\n", OS_rmdir("/NEWDIR/asdfas"));              //-1    invalid path
    printf("main:\t%d\n", OS_rmdir("/NEWDIR"));                     //1     

    entries = OS_readDir("/");
    print_entries(entries);
    free(entries);

    printf("main:\t%d\n", OS_creat("/TEST2.TXT"));                  //1
    fd = OS_open("/TEST2.TXT");
    printf("main:\t%d\n", OS_write(fd, write_buff, strlen(write_buff), 0));
    printf("main:\t%d\n", OS_read(fd, buff, nbytes, 0));
    buff[strlen(write_buff)] = '\0';
    printf("%s\n", buff);

    entries = OS_readDir("/");
    print_entries(entries);
    free(entries);

    return 0;
}
