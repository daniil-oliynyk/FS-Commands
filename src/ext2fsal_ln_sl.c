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

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <libgen.h>


int32_t ext2_fsal_ln_sl(const char *src,
                        const char *dst)
{
    /**
     * TODO: implement the ext2_ln_sl command here ...
     * src and dst are the ln command arguments described in the handout.
     * Approx: 77 lines
     */

    // path_type: 0 --> denotes source, 1 --> target file
    int parent_inode = check_filepath_sl(dst);

    if (parent_inode == -1){
        printf("enoent\n");
        return ENOENT;
    } else if (parent_inode == -2) {
        printf("eisdir\n");
        return EISDIR;
    } else if (parent_inode == -3) {
        printf("eexist\n");
        return EEXIST;
    }

    // Get name of dir to create 
    char *temp = malloc(sizeof(char *) * strlen(dst));
    strcpy(temp, dst);
    char *link_to_create = basename(temp);
    
    int symlink_inode = create_new_link(parent_inode, link_to_create, EXT2_FT_SYMLINK, -1);

    if(symlink_inode == ENOSPC * -1){
        return ENOSPC;
    }

    char *source = malloc(sizeof(char *) * strlen(src));
    strncpy(source, src, strlen(src));
    
    file_copier(NULL, symlink_inode, 0, source);

    return 0;
}
