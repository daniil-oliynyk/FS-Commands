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


int32_t ext2_fsal_rm(const char *path)
{
    /**
     * TODO: implement the ext2_rm command here ...
     * the argument path is the path to the file to be removed.
     * Approx: 79 lines
     */

    // This helper returns the inode of the dir entry thats before the file we want to remove.
    int prev_entry = check_filepath_rm(path);
    printf("inode: %d\n", prev_entry);

    if(prev_entry == -1){
        printf("enoent\n");
        return ENOENT;
    }

    if (prev_entry == -2) {
        printf("eisdir\n");
        return EISDIR;
    }

    return 0;
}