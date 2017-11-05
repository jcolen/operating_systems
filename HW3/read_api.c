/**
*
*
* API for reading a FAT formatted volume
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include "read_api.h"

#define NUM_FD 100

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
char cwd_name[100];         //Stores name of path to current working directory
DIRENTRY * root_entries;    //Stores the DIRENTRY values in the root directory
DIRENTRY * cwd_entries;     //Represents current working directory

int fd_base[NUM_FD];        //Stores the first cluster number of a file at a given file descriptor
DIRENTRY fd_dirEnt[NUM_FD]; //Stores the DIRENTs opened by a file descriptor

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
DIRENTRY * read_cluster_dirEnt(int cluster)    {
    int curr = cluster;
    int sector = (curr - 2) * bpb_struct.BPB_SecPerClus + data_sec;
    int chain_length = cluster_chain_length(cluster);

    printf("read_cluster_dirEnt:\tChain Length:\t%d\n", chain_length);

    int num_entries = chain_length * bpb_struct.BPB_SecPerClus * 
        bpb_struct.BPB_BytsPerSec / sizeof(DIRENTRY);

    DIRENTRY * entries = (DIRENTRY *) malloc(sizeof(DIRENTRY) * num_entries);
    lseek(fat_fd, sector * bpb_struct.BPB_BytsPerSec, SEEK_SET);
    int entry_count = 0;
    while(entry_count < num_entries)    {
        read(fat_fd, (char*)&(entries[entry_count]), sizeof(DIRENTRY));
        if(((char*)&entries[entry_count])[0] == 0)  //First byte 0 means no more
            break;
        printf("read_cluster_dirEnt:\tEntry:\t%.11s\t0x%02x\t%d\t%d\t%d\n", 
            entries[entry_count].DIR_Name,
            entries[entry_count].DIR_Attr,
            entries[entry_count].DIR_FstClusHI,
            entries[entry_count].DIR_FstClusLO,
            entries[entry_count].DIR_FileSize);
        entry_count ++;
         
        //If we've read past the end of the cluster, we need to follow the chain
        if ((entry_count * sizeof(DIRENTRY)) >= 
            (bpb_struct.BPB_BytsPerSec * bpb_struct.BPB_SecPerClus))    {
            curr = value_in_FAT(curr);
            if (fsys_type == 0x01 && curr >= 0xFFF8)
                break;
            if (fsys_type == 0x02 && curr >= 0x0FFFFFF8)
                break;
            sector = (curr - 2) * bpb_struct.BPB_SecPerClus + data_sec;
            lseek(fat_fd, sector * bpb_struct.BPB_BytsPerSec, SEEK_SET);
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
    
    printf("init_fat:\tRoot Sectors: %d Root Entries: %d Bytes Per Sector: %d\n", RootDirSectors, bpb_struct.BPB_RootEntCnt, bpb_struct.BPB_BytsPerSec);

    int FATSz, TotSec;
    if (bpb_struct.BPB_FATSz16 != 0)
        FATSz = bpb_struct.BPB_FATSz16;
    else
        FATSz = ebr_fat32.BPB_FATSz32;

    printf("init_fat:\tFAT Size:\t%d\n", FATSz);

    if (bpb_struct.BPB_TotSec16 != 0)
        TotSec = bpb_struct.BPB_TotSec16;
    else
        TotSec = bpb_struct.BPB_TotSec32;

    printf("init_fat:\tTotal Sectors:\t%d\n", TotSec);

    int DataSec = TotSec - (bpb_struct.BPB_RsvdSecCnt + 
        (bpb_struct.BPB_NumFATs * FATSz) + RootDirSectors);
    int CountofClusters = DataSec / bpb_struct.BPB_SecPerClus;

    printf("init_fat:\tCount of Clusters:\t%d\n", CountofClusters);

    if (CountofClusters < 4085) {   //Volume is FAT12 - exit
        return -1;
    } else if (CountofClusters < 65525) {   //Volume is FAT16
        fsys_type = 0x01;
        printf("init_fat:\tFAT16 Volume\n");
    } else  {   //Volume is FAT32
        fsys_type = 0x02;
        printf("init_fat:\tFAT32 Volume\n");
    }

    //Initialize more global variables
    root_sec = bpb_struct.BPB_RsvdSecCnt +
        (bpb_struct.BPB_NumFATs * FATSz);
    data_sec = root_sec + RootDirSectors;
    strcpy(cwd_name, "/");
   
    printf("init_fat:\tRoot Byte: %x Data Byte: %x\n", root_sec * bpb_struct.BPB_BytsPerSec, data_sec * bpb_struct.BPB_BytsPerSec);

    //Load in root directory
    if (fsys_type == 0x01)  {   //FAT16
        root_entries = (DIRENTRY *) malloc(sizeof(DIRENTRY) *
            bpb_struct.BPB_RootEntCnt);
        lseek(fat_fd, root_sec * bpb_struct.BPB_BytsPerSec, SEEK_SET);
        int entry_count = 0;
        printf("init_fat:\tNumber of Root Entries:\t%d\n", bpb_struct.BPB_RootEntCnt);
        while (entry_count < bpb_struct.BPB_RootEntCnt)  {
            read(fat_fd, (char*)&(root_entries[entry_count]), sizeof(DIRENTRY));
            if (((char*)&root_entries[entry_count])[0] == 0)    //First byte 0 means no more to read
                break;
            printf("init_fat:\tRoot Entry:\t%.11s\t%02x\t%d\t%d\t%d\n",
                root_entries[entry_count].DIR_Name, 
                root_entries[entry_count].DIR_Attr,
                root_entries[entry_count].DIR_FstClusHI,
                root_entries[entry_count].DIR_FstClusLO,
                root_entries[entry_count].DIR_FileSize);
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
* @return 1 if it is found, 0 otherwise 
*/
int findDirEntry(DIRENTRY * dest, const DIRENTRY * current, char * name)    {
    int i = 0;
    char * lfilename = NULL;
    malloc(sizeof(DIRENTRY));
    while(1)    {
        DIRENTRY de = current[i];
        i ++;
        if (de.DIR_Name[0] == 0)    //No more entries in directory
            break;
        if (de.DIR_Name[0] == 0xE5) //Unused entry
            continue;

        if (de.DIR_Attr == 0x0F)    {   //Long filename
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

        printf("findDirEntry:\tEntry Name:\t%.11s\n", de.DIR_Name);

        if (lfilename == NULL)  {   //Parse the short filename
            lfilename = malloc(sizeof(char) * 11);
            lfilename[0] = '\0';    //Null terminate for concatenations
            char * padding = strchr(de.DIR_Name, 0x20); //beginning of padding
            if (padding == NULL)
                strncat(lfilename, de.DIR_Name, 8);
            else
                strncat(lfilename, de.DIR_Name, padding - de.DIR_Name);
            padding = strchr(de.DIR_Name + 8, 0x20); //beginning of padding in extension
            if (padding != de.DIR_Name + 8)
                strcat(lfilename, ".");
            if (padding == NULL)
                strncat(lfilename, de.DIR_Name + 8, 3);
            else
                strncat(lfilename, de.DIR_Name + 8, padding - de.DIR_Name - 8);
        }
        printf("findDirEntry:\tFull File Name:\t%s\n", lfilename);
        
        if(strcmp(lfilename, name) == 0)   {   //Filename match
            free(lfilename);
            *dest = de;
            return 1;
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
DIRENTRY * findDir(const char * path, const DIRENTRY * current)   {
    if (path == NULL || strlen(path) == 0)   //Base case
        return NULL;

    char *first, *remaining;
    first = (char*) malloc(sizeof(char) * strlen(path));
    remaining = (char*) malloc(sizeof(char) * strlen(path));
    first = getFirstElement(first, path);
    remaining = getRemaining(remaining, path);
    printf("findDir:\t%s %s %s\n", path, first, remaining);

    //Locate first element in current directory
    DIRENTRY dirEnt;
    if (!findDirEntry(&dirEnt, current, first))
        return NULL;

    //Load in next directory and recurse down
    int cluster = (dirEnt.DIR_FstClusHI << 2) | dirEnt.DIR_FstClusLO;
    printf("findDir:\tMatch found - Cluster:\t%d\n", cluster);
    DIRENTRY * next_dir = read_cluster_dirEnt(cluster);
    DIRENTRY * ret = findDir(remaining, next_dir);
    free(first);
    free(remaining);
    if (ret == NULL)    //Avoid memory leaks
        return next_dir;
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

    DIRENTRY * current = OS_readDir(path);
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
        free(buff);
    }


    pathname = malloc(sizeof(char) * strlen(path));
    int pathlen = strstr(path, filename) - path;
    strncpy(pathname, path, pathlen);

    printf("OS_open:\tOpening %s in %s\n", filename, pathname);
    
    DIRENTRY * current = OS_readDir(pathname);
    
    DIRENTRY file;
    if (!findDirEntry(&file, current, filename))
        return -1;

    free(current);  //Avoid memory leaks

    //Find the first available file descriptor
    int fd = 0;
    while (fd < NUM_FD && fd_base[fd] != -1)
        fd ++;

    fd_base[fd] = (file.DIR_FstClusHI << 2) | (file.DIR_FstClusLO);
    fd_dirEnt[fd] = file;

    printf("OS_open:\tFile Size:\t%d\n", file.DIR_FileSize);

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
* TODO Figure out if we need to count bytes up to end of cluster/nbytes
* or up to EOF in file if that's first. Or if end of cluster is EOF
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

    printf("OS_read:\tFile Size:\t%d\n", fd_dirEnt[fildes].DIR_FileSize);

    int untilEOF = fd_dirEnt[fildes].DIR_FileSize - bytesRead - offset;
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
* @return An array of DIRENTRYs
*/
DIRENTRY * OS_readDir(const char * dirname)  {
    if (fat_fd == -1)    {   //If directory hasn't been loaded yet
        int err = init_fat();   //Load the FAT volume
        if (err == -1)  
            return NULL;
    }


    DIRENTRY * current = cwd_entries;

    //If absolute path name start path at /
    if (dirname[0] == '/') 
        current = root_entries;

    printf("OS_readDir:\tCurrent is Root:\t%d\n", current == root_entries);

    return findDir(dirname, current);
}
