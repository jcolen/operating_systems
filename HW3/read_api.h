#ifndef READ_API_H_
#define READ_API_H_

#include <stdlib.h>

/**
* Structure representing a directory entry in a FAT filesystem
*/
typedef struct __attribute__((packed)) {
    char dir_name[11];          //Short name
    char dir_attr;              //ATTR_READ_ONLY    0x01
                                //ATTR_HIDDEN       0x02
                                //ATTR_SYSTEM       0x04
                                //ATTR_VOLUME_ID    0x08
                                //ATTR_DIRECTORY    0x10
                                //ATTR_ARCHIVE      0x20
                                //ATTR_LONG_NAME    0x0F
    char dir_NTRes;             //Reserved for Windows NT
    char dir_crtTimeTenth;      //Millsecond stamp at creation time
    short int dir_crtTime;      //Time file was created
    short int dir_crtDate;      //Date file was created
    short int dir_lstAccDate;   //Last access date
    short int dir_fstClusHI;    //High word of entry's first cluster number - 0 for FAT16
    short int dir_wrtTime;      //Time of last write
    short int dir_wrtDate;      //Date of last write
    short int dir_fstClusLO;    //Low word of entry's first cluster numer
    int dir_fileSize;           //32 bit word holding size in bytes
} dirEnt;


/**
* Changes the current working directory to the specified path
* @param path The absolute or relative path of the file
* @return 1 on success, -1 on failure
*/
int OS_cd(const char * path);

/**
* Opens a file specified by path to be read/written to
* @param path The absolute or relative path of the file
* @return The file descriptor to be used, or -1 on failure
*/
int OS_open(const char * path);

/**
* Close an opened file specified by fd
* @param fd The file descriptor of the file to be closed
* @preturn 1 on success, -1 on failure
*/
int OS_close(int fd);

/**
* Read nbytes of a file from offset into buf
* @param fildes A previously opened file
* @param buf A buffer of at least nbyte size
* @param nbyte The number of bytes to read
* @param offset The offset in the file to begin reading
* @return The number of bytes read, or -1 otherwise
*/
int OS_read(int fildes, void * buf, int nbyte, int offset);

/**
* Gives a list of directory entries contained in a directory
* @param dirname The path to the directory
* @return An array of DIRENTRYs
*/
dirEnt * OS_readDir(const char * dirname);

#endif
