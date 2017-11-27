/**
*	Name: 		Jonathan Colen
*	Email:		jc8kf@virginia.edu
*	Class:		CS 4414
*	Professor:	Andrew Grimshaw
*	Assignment:	Machine Problem 3 
*
*   The purpose of this program is to implement an API for reading a 
*   FAT formatted volume. This can be used to navigate through a FAT 
*   filesystem.
*
*   This program can be compiled with fat_api.c via "make".
*/

#ifndef FAT_API_H_
#define FAT_API_H_

#include <stdlib.h>
#include <stdint.h>

/**
* Structure representing a directory entry in a FAT filesystem
*/
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

/**
* Creates a new directory at the specified path
* @param path The path to the directory
* @return 1 if created, -1 if the path is invalid, -2 if final 
*   path element already exists
*/
int OS_mkdir(const char * path);

/**
* Remove a directory at the specified path
* @param path The path to the directory
* @return 1 if removed, -1 if path is invalid,
*   -2 if path does not refer to a file, 
*   -3 if directory is not empty
*/
int OS_rmdir(const char * path);

/**
* Remove a file at the specified path
* @param path The path to the file
* @return 1 if removed, -1 if path is invalid,
*   -2 if file is a directory
*/
int OS_rm(const char * path);

/**
* Create a file at a desired path name
* @param path The path to the file
* @return 1 if file is created, -1 if path is invalid,
*   -2 if final path element already exists
*/
int OS_creat(const char * path);

/**
* Write to an opened file
* @param fildes The file descriptor
* @param buf The buffer of bytes to be written
* @param nbytes The number of bytes to write
* @param offset The offset at which to write
* @return The number of bytes written, or -1 on failure
*/
int OS_write(int fildes, const void * buf, int nbytes, int offset);

#endif
