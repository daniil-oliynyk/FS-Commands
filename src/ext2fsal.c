/*
 *------------
 * This code is provided solely for the personal and private use of
 * students taking the CSC369H5 course at the University of Toronto.
 * Copying for purposes other than this use is expressly prohibited.
 * All forms of distribution of this code, whether as given or with
 * any changes, are expressly prohibited.
 *
 * All of the files in this directory and all subdirectories are:
 * Copyright (c) 2019 MCS @ UTM
 * -------------
 */

// Approx: 69 lines for both functions 

#include "ext2fsal.h"
#include "e2fs.h"

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <pthread.h>

unsigned char *disk;
pthread_mutex_t block_bitmap_lock;
pthread_mutex_t inode_bitmap_lock;
pthread_mutex_t inode_table_lock;
pthread_mutex_t gdt_lock;
pthread_mutex_t sb_lock;

void ext2_fsal_init(const char* image)
{
    /**
     * TODO: Initialization tasks, e.g., initialize synchronization primitives used,
     * or any other structures that may need to be initialized in your implementation,
     * open the disk image by mmap-ing it, etc.
     */


    int fd = open(image, O_RDWR);
    if(fd < 0){
        perror("open"); // Put in appropriate error code if there is
        exit(1);
    }

    disk = mmap(NULL, 128 * 1024, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(disk == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }

    // Init mutex locks
    pthread_mutex_init(&block_bitmap_lock, NULL);
    pthread_mutex_init(&inode_bitmap_lock, NULL);
    pthread_mutex_init(&inode_table_lock, NULL);

}

void ext2_fsal_destroy()
{
    /**
     * TODO: Cleanup tasks, e.g., destroy synchronization primitives, munmap the image, etc.
     */
}