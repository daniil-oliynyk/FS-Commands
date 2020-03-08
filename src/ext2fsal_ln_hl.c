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


int32_t ext2_fsal_ln_hl(const char *src,
                        const char *dst)
{
    /**
     * TODO: implement the ext2_ln_hl command here ...
     * src and dst are the ln command arguments described in the handout.
     * Approx: 76 lines
     */

    // path_type: 0 --> denotes source, 1 --> target file
    int source_inode = check_filepath_hl(src, 0);

    if (source_inode == -1){
        printf("enoent\n");
        return ENOENT;
    } else if (source_inode == -2) {
        printf("eisdir\n");
        return EISDIR;
    }

    int targets_parents_inode = check_filepath_hl(dst, 1);

    if (targets_parents_inode == -1){
        printf("enoent\n");
        return ENOENT;
    } else if (targets_parents_inode == -2) {
        printf("eisdir\n");
        return EISDIR;
    } else if (targets_parents_inode == -3) {
        printf("eexist\n");
        return EEXIST;
    }
    printf("First inode: %d\n", source_inode);
    printf("target parents inode: %d\n", targets_parents_inode);
    
    // Get name of dir to create 
    char *temp = malloc(sizeof(char *) * strlen(dst));
    strcpy(temp,dst);
    char *link_to_create = basename(temp);

    // Create new hard link and denote it with 2
    int err = create_new_link(targets_parents_inode, link_to_create, EXT2_FT_REG_FILE, source_inode);

    if(err == ENOSPC * -1){
        return ENOSPC;
    }

    return 0;
}
