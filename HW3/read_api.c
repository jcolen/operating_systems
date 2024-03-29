/**
*	Name: 		Jonathan Colen
*	Email:		jc8kf@virginia.edu
*	Class:		CS 4414
*	Professor:	Andrew Grimshaw
*	Assignment:	Machine Problem 3 
*
*   The purpose of this program is to implement an API for reading a 
*   FAT formatted volume. This can be used to navigate through a FAT 
*   filesystem. This particular file implements read operations 
*   such as cd, open, read, close, and readDir, which is like ls
*   
*   This program can be compiled with read_api.h via "make".
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include "read_api.h"

#define NUM_FD 100

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
* Global variables
*/

int fat_fd = -1;
char fsys_type = 0;         //0x01 for FAT16, 0x02 for FAT32
BPB_Structure bpb_struct;   //Stores the bios partition block once the volume is loaded
EBR_FAT16 ebr_fat16;        //Stores the extended boot record for FAT16 volumes
EBR_FAT32 ebr_fat32;        //Stores the extended boot record for FAT32 volumes
int root_sec;               //Sector of the Root directory
int data_sec;               //Sector of the data section after Root
dirEnt * root_entries;    //Stores the dirENTRY values in the root directory
dirEnt * cwd_entries;     //Represents current working directory

int fd_base[NUM_FD];        //Stores the first cluster number of a file at a given file descriptor
dirEnt fd_dirEnt[NUM_FD]; //Stores the dirENTs opened by a file descriptor

/**
* Given a valid cluster number N, where is the offset in the FAT?
* NOTE: Given the return value FATOffset:
*   FATSecNum = BPB_RsvdSecCnt + (FATOffset / BPB_BytsPerSec);
*   FATEntOffset = FATOffset % BPB_BytsPerSec
* @param clus_nbr The cluster number
* @return The index in the FAT for that cluster number
*/
int offset_in_FAT(int clus_nbr)  {
    int FATSz, FATOffset;
    if (bpb_struct.BPB_FATSz16 != 0)
        FATSz = bpb_struct.BPB_FATSz16;
    else
        FATSz = ebr_fat32.BPB_FATSz32;

    if (fsys_type == 0x01)  //If FAT16
        FATOffset = clus_nbr * 2;
    else
        FATOffset = clus_nbr * 4;

    return FATOffset;
}

/**
* Given a FAT cluster, return the FAT entry at that cluster
* @param cluster The cluster number
* @return The value in the FAT
*/
int value_in_FAT(int cluster)    {
    int offset = offset_in_FAT(cluster);
    //Calculate the sector number and offset in that sector
    int FATSecNum = bpb_struct.BPB_RsvdSecCnt + 
        (offset / bpb_struct.BPB_BytsPerSec);
    int FATEntOffset = offset % bpb_struct.BPB_BytsPerSec;

    //Now lseek to the sector and load it into an array
    char sec_buffer[bpb_struct.BPB_BytsPerSec];
    lseek(fat_fd, FATSecNum * bpb_struct.BPB_BytsPerSec, SEEK_SET);
    read(fat_fd, sec_buffer, bpb_struct.BPB_BytsPerSec);

    //Locate the value in the FAT buffer
    int val_FAT;
    if (fsys_type == 0x01)
        val_FAT = *((unsigned short int *) &sec_buffer[FATEntOffset]);
    else
        val_FAT = (*((unsigned int *) &sec_buffer[FATEntOffset])) & 0x0FFFFFFF;

    return val_FAT;
}

/**
* Given a cluster number, how many clusters does it chain to?
* @param cluster The cluster number to be checked
* @return The number of clusters in the chain
*/
int cluster_chain_length(int cluster)   {
    int count = 1;
    int curr = cluster;
    int flag = 1;
    while(flag) {
        int next = value_in_FAT(curr);
        if (fsys_type == 0x01 && next >= 0xFFF8)  {
            flag = 0;
            continue;
        } 
        if (fsys_type == 0x02 && next >= 0x0FFFFFF8)  {
            flag = 0;
            continue;
        }
        curr = next;
        count ++;
    }

    return count;
}

/**
* Read in directory entries from a cluster and follow the cluster chain
* @param cluster The cluster number to be read
* @return The list of directory entries in a cluster
*/
dirEnt * read_cluster_dirEnt(int cluster)    {
    int curr = cluster;
    int sector = (curr - 2) * bpb_struct.BPB_SecPerClus + data_sec;

    //cluster number of 0 is the root directory
    if (curr == 0)  {
        if (fsys_type == 0x01)  {
            sector = root_sec;
        } else if (fsys_type == 0x02)   {
            curr = ebr_fat32.BPB_RootClus;
            sector = (curr - 2) * bpb_struct.BPB_SecPerClus + data_sec;
        }
    }

    int chain_length = cluster_chain_length(cluster);
    
    int num_entries = chain_length * bpb_struct.BPB_SecPerClus * 
        bpb_struct.BPB_BytsPerSec / sizeof(dirEnt);

    dirEnt * entries = (dirEnt *) malloc(sizeof(dirEnt) * num_entries);
    lseek(fat_fd, sector * bpb_struct.BPB_BytsPerSec, SEEK_SET);
    int entry_count = 0;    //Tracks the number of entries read in
    int cluster_count = 0;  //Tracks the number of entries in the current cluster
    while(entry_count < num_entries)    {
        read(fat_fd, (char*)&(entries[entry_count]), sizeof(dirEnt));
        if(((char*)&entries[entry_count])[0] == 0)  //First byte 0 means no more
            break;
        entry_count ++;
        cluster_count ++;
        //If we've read past the end of the cluster, we need to follow the chain
        if ((cluster_count * sizeof(dirEnt)) >= 
            (bpb_struct.BPB_BytsPerSec * bpb_struct.BPB_SecPerClus))    {
            curr = value_in_FAT(curr);
            if (fsys_type == 0x01 && curr >= 0xFFF8)
                break;
            if (fsys_type == 0x02 && curr >= 0x0FFFFFF8)
                break;
            sector = (curr - 2) * bpb_struct.BPB_SecPerClus + data_sec;
            lseek(fat_fd, sector * bpb_struct.BPB_BytsPerSec, SEEK_SET);
            cluster_count = 0;
        }
    }

    return entries;
}

/**
* Initialize the FAT volume and load all relevant data
*/
int init_fat()   {
    char * basedir = getenv("FAT_FS_PATH");   //Directory of FAT16 volume
    if (basedir == NULL)    {
        return -1;
    }

    fat_fd = open(basedir, O_RDWR, 0);    //Store the file descriptor
    
    if (fat_fd == -1)   {
        return -1;
    }

    //Read in the BPB_Structure
    read(fat_fd, (char*)&bpb_struct, sizeof(BPB_Structure));
    read(fat_fd, (char*)&ebr_fat16, sizeof(EBR_FAT16)); //Load EBR for FAT16
    lseek(fat_fd, sizeof(BPB_Structure), SEEK_SET);    //Reset offset for FAT32
    read(fat_fd, (char*)&ebr_fat32, sizeof(EBR_FAT32)); //Load EBR for FAT32

    //Determine FAT16 or FAT32
    //Number of sectors occupied by root directory
    int RootDirSectors = ((bpb_struct.BPB_RootEntCnt * 32) + 
        (bpb_struct.BPB_BytsPerSec -1)) / bpb_struct.BPB_BytsPerSec;
    

    int FATSz, TotSec;
    if (bpb_struct.BPB_FATSz16 != 0)
        FATSz = bpb_struct.BPB_FATSz16;
    else
        FATSz = ebr_fat32.BPB_FATSz32;

    if (bpb_struct.BPB_TotSec16 != 0)
        TotSec = bpb_struct.BPB_TotSec16;
    else
        TotSec = bpb_struct.BPB_TotSec32;

    int DataSec = TotSec - (bpb_struct.BPB_RsvdSecCnt + 
        (bpb_struct.BPB_NumFATs * FATSz) + RootDirSectors);
    int CountofClusters = DataSec / bpb_struct.BPB_SecPerClus;

    if (CountofClusters < 4085) {   //Volume is FAT12 - exit
        return -1;
    } else if (CountofClusters < 65525) {   //Volume is FAT16
        fsys_type = 0x01;
    } else  {   //Volume is FAT32
        fsys_type = 0x02;
    }

    //Initialize more global variables
    root_sec = bpb_struct.BPB_RsvdSecCnt +
        (bpb_struct.BPB_NumFATs * FATSz);
    data_sec = root_sec + RootDirSectors;
   
    //Load in root directory
    if (fsys_type == 0x01)  {   //FAT16
        root_entries = (dirEnt *) malloc(sizeof(dirEnt) *
            bpb_struct.BPB_RootEntCnt);
        lseek(fat_fd, root_sec * bpb_struct.BPB_BytsPerSec, SEEK_SET);
        int entry_count = 0;
        while (entry_count < bpb_struct.BPB_RootEntCnt)  {
            read(fat_fd, (char*)&(root_entries[entry_count]), sizeof(dirEnt));
            if (((char*)&root_entries[entry_count])[0] == 0)    //First byte 0 means no more to read
                break;
            entry_count ++;
        }
    } else if (fsys_type == 0x02)   {   //FAT32
        //Root is read like any other cluster
        root_entries = read_cluster_dirEnt(ebr_fat32.BPB_RootClus);
    }

    //Initialize cwd to root
    cwd_entries = root_entries;

    //Free all file descriptors except for 0, 1 (Those are stdin, stdout by convention)
    int i;
    for(i = 0; i < NUM_FD; i ++)    {
        fd_base[i] = -1;
    }

    fd_base[0] = 0;
    fd_base[1] = 0;

    return 1;
}

/**
* Get the first element in the path
* For example, /home/grimshaw/data -> home
* @param dest The pointer in which the element should be placed
* @param path The path to be analyzed
* @return The first element in the path
*/
char * getFirstElement(char * dest, const char * path)   {
    if (path == NULL || strlen(path) == 0)
        return NULL;

    char * slash1 = strchr(path, '/');
    if (slash1 == NULL) {
        strcpy(dest, path);
        return dest;
    }
        

    if (slash1 - path == 0)    {
        char * slash2 = strchr(slash1 + 1, '/');
        if (slash2 == NULL) {
            strcpy(dest, path + 1);
            return dest;
        }
        strncpy(dest, slash1 + 1, slash2 - slash1 - 1);
        dest[slash2-slash1 - 1] = '\0';
        return dest;
    }
    strncpy(dest, path, slash1-path);
    dest[slash1-path] = '\0';
    return dest;
}

/**
* Get the remaining elements in the path
* For example, /home/grimshaw/data -> grimshaw/data
* @param dest The point in which the remaining elements should be placed
* @param path The path to be analyzed
* @return A pointer to dest
*/
char * getRemaining(char * dest, const char * path)  {
    if (path == NULL || strlen(path) == 0)
        return NULL;

    char * slash1 = strchr(path, '/');
    if (slash1 == NULL)  {
        return NULL;
    }

    if (slash1 - path == 0) {
        slash1 = strchr(slash1 + 1, '/');
    }

    if (slash1 == NULL) {
        return NULL;
    }

    int num_cpy = strlen(path) - (slash1 + 1 - path);
    strncpy(dest, slash1 + 1, num_cpy);
    dest[num_cpy] = '\0';
    return dest;
}

void wide_strcat(char * dest, const short int * source, int len) {
    char buff[len + 1];
    buff[len] = '\0';
    int i;
    for(i = 0; i < len; i ++)   {
        if(source[i] == 0x0000 || source[i] == -1)  {   //Padded with -1 or null terminated
            buff[i] = '\0';
            break;
        }
        buff[i] = (char)(source[i]);
    }
    strcat(dest, buff);
}

/**
* Find a directory entry matching a desired name.
* @param dest The destination for the matching directory entry
* @param current The current set of directory entries
* @param name The name to be matched
* @param directory 1 if the entry searched for must be a directory
* @return 1 if it is found, 0 otherwise 
*/
int findDirEntry(dirEnt * dest, const dirEnt * current, char * name, int directory)    {
    int i = 0;
    char * lfilename = NULL;
    while(1)    {
        dirEnt de = current[i];
        i ++;
        if (de.dir_name[0] == 0)    //No more entries in directory
            break;
        if (de.dir_name[0] == 0xE5) //Unused entry
            continue;

        if (de.dir_attr == 0x0F)    {   //Long filename
            //Read in long filename
            if(lfilename == NULL)   {
                lfilename = malloc(sizeof(char) * 255); //FAT spec says max length is 255
                lfilename[0] = '\0';    //Null terminate for concatenations
            }

            LDIR ldir = *(LDIR*)&de;
            wide_strcat(lfilename, ldir.LDIR_Name1, 5);
            wide_strcat(lfilename, ldir.LDIR_Name2, 6);
            wide_strcat(lfilename, ldir.LDIR_Name3, 2);
            continue;
        }

        if (lfilename == NULL)  {   //Parse the short filename
            lfilename = malloc(sizeof(char) * 11);
            lfilename[0] = '\0';    //Null terminate for concatenations
            char * padding = strchr((char*)(de.dir_name), 0x20); //beginning of padding
            if (padding == NULL || padding - (char*)(de.dir_name) > 8)
                strncat(lfilename, (char*)(de.dir_name), 8);
            else
                strncat(lfilename, (char*)(de.dir_name), padding - (char*)(de.dir_name));
            padding = strchr((char*)(de.dir_name) + 8, 0x20); //beginning of padding in extension
            if (padding != (char*)(de.dir_name) + 8)
                strcat(lfilename, ".");
            if (padding == NULL)
                strncat(lfilename, (char*)(de.dir_name) + 8, 3);
            else
                strncat(lfilename, (char*)(de.dir_name) + 8, padding - (char*)(de.dir_name) - 8);
        }
       
        if(strcmp(lfilename, name) == 0)   {   //Filename match
            if (!directory || (directory && de.dir_attr & 0x10))    {
                free(lfilename);
                *dest = de;
                return 1;
            }
        }

        free(lfilename);
        lfilename = NULL;
    }

    return 0;
}

/**
* Given a directory name and a current working directory, locate
* a list of entries contained in the named directory.
* @param path The path to the directory
* @param current The current directory
* @return A list of directory entries, or NULL if it doesn't exist
*/
dirEnt * findDir(const char * path, const dirEnt * current)   {
    if (path == NULL || strlen(path) == 0)   //Base case
        return NULL;

    char *first, *remaining;
    first = (char*) malloc(sizeof(char) * strlen(path));
    remaining = (char*) malloc(sizeof(char) * strlen(path));
    first = getFirstElement(first, path);
    remaining = getRemaining(remaining, path);

    //Locate first element in current directory
    dirEnt dir_Ent;
    if (!findDirEntry(&dir_Ent, current, first, 1)) {
        return NULL;
    }

    //Load in next directory and recurse down
    int cluster = (dir_Ent.dir_fstClusHI << 2) | dir_Ent.dir_fstClusLO;
    dirEnt * next_dir = read_cluster_dirEnt(cluster);
    dirEnt * ret = findDir(remaining, next_dir);
    int no_more_path = (remaining == NULL || strlen(remaining) == 0);
    free(first);
    free(remaining);
    if (ret == NULL && no_more_path)    {
        return next_dir;
    }
    free(next_dir);
    return ret;
}

/**
* Changes the current working directory to the specified path
* @param path The absolute or relative path of the file
* @return 1 on success, -1 on failure
*/
int OS_cd(const char * path)    {
    if(path == NULL || strlen(path) == 0)    {
        return -1;
    }

    if (fat_fd == -1)    {   //If directory hasn't been loaded yet
        int err = init_fat();   //Load the FAT volume
        if (err == -1)  
            return -1;
    }

    dirEnt * current = OS_readDir(path);
    if (current == NULL)
        return -1;

    if (cwd_entries != root_entries && cwd_entries != current)
        free(cwd_entries);

    cwd_entries = current;
    return 1;
}

/**
* Opens a file specified by path to be read/written to
* @param path The absolute or relative path of the file
* @return The file descriptor to be used, or -1 on failure
*/
int OS_open(const char * path)  {
    if (fat_fd == -1)    {   //If directory hasn't been loaded yet
        int err = init_fat();   //Load the FAT volume
        if (err == -1)  
            return -1;
    }

    //Get the file name and the path name
    char * filename = malloc(sizeof(char) * strlen(path));
    char * pathname = malloc(sizeof(char) * strlen(path));
    char * buff = malloc(sizeof(char) * strlen(path));
    pathname = getRemaining(pathname, path);
    filename = getFirstElement(filename, path);

    while (pathname != NULL) {
        buff = pathname;
        filename = getFirstElement(filename, buff);
        pathname = getRemaining(pathname, buff);
    }

    free(buff);

    pathname = malloc(sizeof(char) * strlen(path));
    int pathlen = strstr(path, filename) - path;
    strncpy(pathname, path, pathlen);
    pathname[pathlen] = '\0';

    dirEnt * current = OS_readDir(pathname);
   
    dirEnt file;
    if (!findDirEntry(&file, current, filename, 0))
        return -1;

    free(current);  //Avoid memory leaks

    //Find the first available file descriptor
    int fd = 0;
    while (fd < NUM_FD && fd_base[fd] != -1)
        fd ++;

    fd_base[fd] = (file.dir_fstClusHI << 2) | (file.dir_fstClusLO);
    fd_dirEnt[fd] = file;

    return fd;
}

/**
* Close an opened file specified by fd
* @param fd The file descriptor of the file to be closed
* @preturn 1 on success, -1 on failure
*/
int OS_close(int fd) {
    if (fat_fd == -1)    {   //If directory hasn't been loaded yet
        int err = init_fat();   //Load the FAT volume
        if (err == -1)  
            return -1;
    }

    if (fd < 0 || fd > NUM_FD || fd_base[fd] == -1)
        return -1;

    
    fd_base[fd] = -1;
    
    return 1;
}

/**
* Read nbytes of a file from offset into buf
* @param fildes A previously opened file
* @param buf A buffer of at least nbyte size
* @param nbyte The number of bytes to read
* @param offset The offset in the file to begin reading
* @return The number of bytes read, or -1 otherwise
*/
int OS_read(int fildes, void * buf, int nbyte, int offset)  {
    if (fat_fd == -1)    {   //If directory hasn't been loaded yet
        int err = init_fat();   //Load the FAT volume
        if (err == -1)  
            return -1;
    }

    if (fildes < 0 || fildes > NUM_FD || fd_base[fildes] == -1)
        return -1;

    int bytesPerClus = bpb_struct.BPB_SecPerClus * bpb_struct.BPB_BytsPerSec;

    //Calculate offset in cluster and how many links on cluster chain to follow
    int cluster_offset = offset % bytesPerClus;
    int cluster_num = offset / bytesPerClus; 
    int cluster = fd_base[fildes];

    int count = 0;  //Count tracks the number of cluster chains reached
    while(count < cluster_num)  {
        int next = value_in_FAT(cluster);
        if (fsys_type == 0x01 && cluster >= 0xFFF8)
            break;
        if (fsys_type == 0x02 && cluster >= 0x0FFFFFF8)
            break;
        cluster = next;
        count ++;
    }

    if (count < cluster_num)    //Reached end of cluster chain before offset
        return -1;

    int bytesRead = 0;   //Tracks the number of bytes read
    count = -1;

    //Seek to sector of cluster and cluster offset
    int sector = (cluster - 2) * bpb_struct.BPB_SecPerClus + data_sec;
    lseek(fat_fd, sector * bpb_struct.BPB_BytsPerSec + cluster_offset, SEEK_SET);

    int untilEOF = fd_dirEnt[fildes].dir_fileSize - bytesRead - offset;
    int untilEOC = bytesPerClus - cluster_offset - bytesRead;

    //Break if we've read the number of bytes or we count 0 bytes read -> end of file
    while (bytesRead < nbyte && count != 0 && untilEOF > 0)  {
        //Read either nbytes or until end of cluster or until end of file
        if (untilEOF < untilEOC && untilEOF < (nbyte - bytesRead))  {
            count = read(fat_fd, buf + bytesRead, untilEOF);
        } else if (untilEOF > (nbyte - bytesRead) && (nbyte - bytesRead) < untilEOC)    {
            count = read(fat_fd, buf + bytesRead, nbyte - bytesRead);
        } else  {
            count = read(fat_fd, buf + bytesRead, untilEOC);
            //Go to next cluster
            cluster = value_in_FAT(cluster);
            if (fsys_type == 0x01 && cluster >= 0xFFF8)
                break;
            if (fsys_type == 0x02 && cluster >= 0x0FFFFFF8)
                break;
            sector = (cluster - 2) * bpb_struct.BPB_SecPerClus + data_sec;
            lseek(fat_fd, sector * bpb_struct.BPB_BytsPerSec, SEEK_SET);
            cluster_offset = 0;
            untilEOC = bytesPerClus + count;
        }
        bytesRead += count;
        untilEOF -= count;
        untilEOC -= count;
    }

    return bytesRead;
}

/**
* Gives a list of directory entries contained in a directory
* @param dirname The path to the directory
* @return An array of dirEnts
*/
dirEnt * OS_readDir(const char * dirname)  {
    if (fat_fd == -1)    {   //If directory hasn't been loaded yet
        int err = init_fat();   //Load the FAT volume
        if (err == -1)  
            return NULL;
    }

    dirEnt * current = cwd_entries;

    const char * truncated_dirname;

    //If absolute path name start path at /
    if (dirname[0] == '/') {
        current = root_entries;
        truncated_dirname = dirname + 1;
    } else  {
        truncated_dirname = dirname;
    }

    dirEnt * ret = findDir(truncated_dirname, current);
    if (ret == NULL && strlen(truncated_dirname) == 0)  {
        return current;
    }
    else if (ret == NULL)   {
        return NULL;
    }
    return ret;
}
