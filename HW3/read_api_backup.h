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
* Structure representing a long directory entry name
*/
typedef struct __attribute__((__packed__)) FAT_LONG_DIRENTRY    {
    char LDIR_Ord;              //Order of this entry in the sequence. 0x4N means last in the sequence
    short int LDIR_Name1[5];    //First 5 2-byte characters in this subcomponent
    char LDIR_Attr;             //Must be ATTR_LONG_NAME
    char LDIR_Type;             //0 implies sub-component of long name
    char LDIR_Chksum;           //Checksum of name in short dir name
    short int LDIR_Name2[6];    //Characters 6-11 of subcomponent
    short int LDIR_FstClusLO;   //Must be ZERO
    short int LDIR_Name3[2];    //Characters 12-13 of subcomponent
} LDIR;

/**
* Structure representing the Bios Partition Block
* NOTE: Without __attribute__((__packed__)), the size of this struct
* is set to 40 instead of 36 and the BPB is not loaded correctly
* NOTE: All values are stored in little-endian format, so they must 
* be converted to big_endian upon loading
*/
typedef struct __attribute__((__packed__)) FAT_BS_BPB_Structure {
    char BS_jmpBoot[3];
    char BS_OEMName[8];
    short int BPB_BytsPerSec;   //Number of bytes per sector
    char BPB_SecPerClus;        //Number of sectors per cluster (power of two)
    short int BPB_RsvdSecCnt;   //Number of reserved sectors in the reserved region
    char BPB_NumFATs;           //The number of FAT data structures - 2 for redundancy
    short int BPB_RootEntCnt;   //The number of 32-byte directory entries in the root directory
    short int BPB_TotSec16;     //16-bit total count of sectors on the volume (0 for FAT32)
    char BPB_Media;             //0xF8 is standard value for fixed media. 0xF0 is for removable
    short int BPB_FATSz16;      //16-bit count of sectors occupied by a single FAT (0 for FAT32)
    short int BPB_SecPerTrk;    //Sectors per track for the read operation
    short int BPB_NumHeads;     //Number of heads for the read operation 
    int BPB_HiddSec;            //Number of hidden sectors preceding partition containing FAT volume
    int BPB_TotSec32;           //32-bit count of sectors on the volume
} BPB_Structure;

/**
* Extended Boot Record for FAT16 volumes
*/
typedef struct __attribute__((__packed__)) FAT16_BS_EBR_Structure   {
    char BS_DrvNum;
    char BS_Reserved1;
    char BS_BootSig;
    int BS_VolID;
    char BS_VolLab[11];
    char BS_FilSysType[8];
} EBR_FAT16;

/**
* Extended Boot Record for FAT32 volumes
*/
typedef struct __attribute__((__packed__)) FAT32_BS_EBR_Structure   {
    int BPB_FATSz32;
    short int BPB_ExtFlags;
    short int BPB_FSVer;
    int BPB_RootClus;
    short int BPB_FSInfo;
    short int BPB_BkBootSec;
    char BPB_Reserved[12];
    char BS_DrvNum;
    char BS_Reserved1;
    char BS_BootSig;
    int BS_VolID;
    char BS_VolLab[11];
    char BS_FilSysType[8];
} EBR_FAT32;

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
