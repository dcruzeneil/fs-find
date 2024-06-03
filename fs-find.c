#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include </usr/src/sys/ufs/ffs/fs.h>
#include </usr/src/sys/ufs/ufs/dinode.h>
#include </usr/src/sys/ufs/ufs/dir.h>

/* CONSTANT Definitions */
#define INODE_SIZE 256
#define FRAGMENT_SIZE 4096
#define BLOCK_SIZE 32768
#define SUPER_BLOCK_OFFSET 65536 // dumpfs /dev/ada1p1: superblock location (64k * 1024)
#define ROOT_LVL_FORMATTING 0

/* Function Prototypes */
void traverseDirectoryDirectBlocks(char*, struct fs*, int64_t, int);
void traverseDirectoryIndirectBlocks(struct ufs2_dinode*, int64_t, char*, struct fs*, int);
void traverseDirectorySecondIndirectBlocks(struct ufs2_dinode*, int64_t, char*, struct fs*, int);
int64_t printFiles(char*, struct fs*, struct direct*, int, int64_t);
struct ufs2_dinode* inodeAddress(char*, struct fs*, int);
struct direct* directoryAddress(char*, int64_t);

int 
main(int argc, char *argv[]){
    /* opening the raw disk img */
    int fd = open(argv[1], O_RDONLY);
    if(fd == -1){
        perror("open");
        exit(1);
    }

    /* getting the size of the partition */
    long DISK_PARTITION_SIZE;
    struct stat file;
    if(stat(argv[1], &file) < 0){
        perror("stat");
        exit(1);
    }
    DISK_PARTITION_SIZE = file.st_size;

    /* mapping the disk-partition to memory*/
    char *addr = mmap(NULL, DISK_PARTITION_SIZE, PROT_READ, MAP_PRIVATE, fd, 0);
    if(addr == (void*) -1){
        perror("mmap");
        exit(1);
    }

    /* identifying the superblock */
    // jumping to the location of the superblock: memory address here is a pointer of type struct fs
    struct fs *ptrSuperBlock = (struct fs*) (addr + SUPER_BLOCK_OFFSET); 

    /* traversing the direct blocks of directory: starting at the root inode */
    traverseDirectoryDirectBlocks(addr, ptrSuperBlock, UFS_ROOTINO, ROOT_LVL_FORMATTING);
}

void 
traverseDirectoryDirectBlocks(char* address, struct fs *superblock, int64_t inodeNumber, int level){
    /* identifying the inode address */
    struct ufs2_dinode *inode = inodeAddress(address, superblock, inodeNumber);
    int64_t sizeBytes = inode->di_size;
    
    // traversing the direct blocks of inode struct
    for(int i = 0; i < UFS_NDADDR; i++){
        // we are done traversing
        if(sizeBytes <= 0){
            break;
        }
        struct direct *dir = directoryAddress(address, inode->di_db[i]);
        // printing the file names (direct struct) at the directory address
        sizeBytes = sizeBytes - printFiles(address, superblock, dir, level, sizeBytes);;
    }
    // traversing the indirect blocks: if we still have bytes to read
    if(sizeBytes > 0){
        traverseDirectoryIndirectBlocks(inode, sizeBytes, address, superblock, level);
    }
}

void 
traverseDirectoryIndirectBlocks(struct ufs2_dinode *inode, int64_t sizeLeft, char *address, struct fs *superblock, int level){
    // identifying the di_db that the di_ib points to
    int64_t *db = (int64_t*)(address + (inode->di_ib[0] * FRAGMENT_SIZE));
    int64_t dbAddress = (int64_t) db;
    int64_t dbBeginning = dbAddress
    for(; dbAddress < dbBeginning + BLOCK_SIZE; dbAddress = dbAddress + sizeof(int64_t)){
        db = (int64_t*) dbAddress;
        // we are done traversing
        if(sizeLeft <= 0){
            break;
        }
        struct direct *dir = directoryAddress(address, *db);
        
        sizeLeft = sizeLeft - printFiles(address, superblock, dir, level, sizeLeft);
    }
    if(sizeLeft > 0){
        traverseDirectorySecondIndirectBlocks(inode, sizeLeft, address, superblock, level);
    }
}

void 
traverseDirectorySecondIndirectBlocks(struct ufs2_dinode *inode, int64_t sizeLeft, char *address, struct fs *superblock, int level){
    int64_t *ib = (int64_t*)(address + (inode->di_ib[1] * FRAGMENT_SIZE));
    int64_t ibAddress = (int64_t) ib;
    int64_t ibBeginning = ibAddress;
    for(; ibAddress < ibBeginning + BLOCK_SIZE; ibAddress = ibAddress + sizeof(int64_t)){
        ib = (int64_t*) ibAddress;
        int64_t *db = (int64_t*)(address + (*ib * FRAGMENT_SIZE));
        int64_t dbAddress = (int64_t) db;
        int64_t dbBeginning = dbAddress;
        for(; dbAddress < dbBeginning + BLOCK_SIZE; dbAddress = dbAddress + sizeof(int64_t)){
            db = (int64_t*) dbAddress;
            // we are done traversing
            if(sizeLeft <= 0){
                break;
            }
            struct direct *dir = directoryAddress(address, *db);
            sizeLeft = sizeLeft - printFiles(address, superblock, dir, level, sizeLeft);
        }
    }
}

int64_t 
printFiles(char *address, struct fs *superblock, struct direct *dir, int level, int64_t size){
    int64_t dirBeginning = (int64_t) dir;
    struct direct *currentDirPointer = dir;
    int64_t sizeTraverse = size > BLOCK_SIZE ? BLOCK_SIZE : size;
    int64_t currentDir;
    int64_t bytesRead;

    // each address in di_db points to a fragment of memory or block of memory, but maybe we did not use all of it
    for(currentDir = dirBeginning; currentDir < dirBeginning + sizeTraverse; currentDir = currentDir + currentDirPointer->d_reclen){
        currentDirPointer = (struct direct*) currentDir;

        // we do not have to print . and ..
        if(strcmp(currentDirPointer->d_name, ".") != 0 && strcmp(currentDirPointer->d_name, "..") != 0){
            // for formatting 
            for(int j = 0; j < level; j++){
                printf("  ");
            }
            printf("%s\n", currentDirPointer->d_name);
    
            if(currentDirPointer->d_type == 4){
                // if directory grab inode and repeat process
                traverseDirectoryDirectBlocks(address, superblock, currentDirPointer->d_ino, level + 1);
            }
        }
    }

    bytesRead = currentDir - dirBeginning;

    return bytesRead;
}

struct ufs2_dinode*
inodeAddress(char* address, struct fs *sb, int inodeNumber){
    int64_t blockAddress = ino_to_fsba(sb, inodeNumber);
    int64_t blockOffset = ino_to_fsbo(sb, inodeNumber);
    struct ufs2_dinode *inode = (struct ufs2_dinode*) (address + (FRAGMENT_SIZE * blockAddress) + (INODE_SIZE * blockOffset));
    return inode;
}

struct direct*
directoryAddress(char *address, int64_t directoryBlock){
    int64_t directoryOffset = (FRAGMENT_SIZE * directoryBlock);
    struct direct *directory = (struct direct*)(address + directoryOffset);
    return directory;
}