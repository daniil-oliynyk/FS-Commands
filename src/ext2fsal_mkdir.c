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

#include "ext2fsal.h"
#include "e2fs.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <libgen.h>



int32_t ext2_fsal_mkdir(const char *path)
{
    /**
     * TODO: implement the ext2_mkdir command here ...
     * the argument path is the path to the directory to be created.
     * Approx: 68 lines
     */
    struct ext2_group_desc *gdt = (struct ext2_group_desc *)(disk + EXT2_BLOCK_SIZE * 2);

    // -1 denotes ENONENT
    int parent_inode = check_filepath_mkdir(path);
        printf("check returned %d\n", parent_inode);

    if(parent_inode == -1){
        return ENOENT;
    }
    if(parent_inode == -2){
        return ENOMEM;
    } 

    char *temp = malloc(sizeof(char *) * strlen(path));
    strcpy(temp,path);
    char *dir_to_create = basename(temp);

    // Create a new directory so denote by 0
    int err = create_new_directory_or_file(parent_inode, dir_to_create, EXT2_S_IFDIR, 0);
    
    if(err == ENOSPC * -1){
        return ENOSPC;
    }

    gdt->bg_used_dirs_count++;
    return 0;
}