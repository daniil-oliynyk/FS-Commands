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



int32_t ext2_fsal_cp(const char *src,
                     const char *dst)
{
    /**
     * TODO: implement the ext2_cp command here ...
     * src and dst are the ln command arguments described in the handout.
     * Approx: 74 lines
     */

    // Verify source file path. Tries to open file, if it does not exist (path invalid) then return ENOENT
    FILE* sourcefile = fopen(src, "rb");
    if(sourcefile == NULL){
        return ENOENT;
    }

    int inode_num = check_filepath_cp(dst);
    printf("parent inode: %d\n", inode_num);

    if(inode_num == -1){
        printf("enoent\n");
        return ENOENT;
    }

    char *temp = malloc(sizeof(char *) * strlen(dst));
    if(temp == NULL){
        return ENOSPC;
    }
    strcpy(temp,dst);
    char *dir_to_create = basename(temp);
        
    // Case 1: for cp, so given parent directory denote parent_inode by third parameter inode_type
    if(inode_num < -1){
        // We know we are given a parent inode
        int parent_inode = abs(inode_num);

        //create file blah, given parent directory so denote by 1
        int new_inode = create_new_directory_or_file(parent_inode, dir_to_create, EXT2_S_IFREG, 1); 
        printf("err is %d\n", new_inode);


        if(new_inode == ENOSPC * -1){
            return ENOSPC;
        }

        file_copier(sourcefile,new_inode, 0, NULL);

    } else {
        // Cases 3, 4, 5, & 6 given existing /blah and direct inode
        int inode_type = inode_type_checker(inode_num);
        
        // Given a inode block that is a file
        if (inode_type == 0){ // Case 3: overwrite file blah 
            printf("Overwrite the blah file: Case 3\n");
            // TODO: clear/free first, THEN insert copy helper here
            int block_index = free_blocks(inode_num, 0);

            if (block_index == ENOSPC * -1) {
                return ENOSPC;
            } else {
                file_copier(sourcefile, inode_num, 0, NULL);
                return 0;
            }
        } else { //  Case 4, 5, 6: A directory, we don't know if doh.txt exists or not
            // Check if file already exists, if it does overwrite it otherwise create it and then write to it

            // Get Name of file to copy
            char *temp = malloc(sizeof(char *) * strlen(src));
            if(temp == NULL){
                return ENOSPC;
            }
            strcpy(temp,src);
            char *file_to_copy = basename(temp);

            // Call Helper
            int walk_copy = existing_file_walker(inode_num, file_to_copy, sourcefile);

            if (walk_copy == -1) {
                return ENOSPC;
            } else {
                return 0;
            }
            
        }
        
    }
    
    fclose(sourcefile);

    return 0;
}