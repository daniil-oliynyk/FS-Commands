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

#include "e2fs.h"


#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>
#include <time.h>
#include <pthread.h>

 /**
  * Add any helper implementations here.
  * The purpose of this file is for all the helper(s) to use
  * which might include getting directory (e.g. f, l, d) etc..
  * Make sure to put any headers into 'e2fs.h'
  * Approx: 439 lines
  */

unsigned char *disk;
pthread_mutex_t block_bitmap_lock;
pthread_mutex_t inode_bitmap_lock;
pthread_mutex_t inode_table_lock;
pthread_mutex_t gdt_lock;
pthread_mutex_t sb_lock;

unsigned char filetype_helper(unsigned char type) {
    // Symbolic Link
    if (type == 3) {
        return 'l';
    } else if (type == 2) {
        return 'd';
    } else {
        return 'f';
    }
}


int check_filepath_cp(const char *path){

    char *filepath = strdup(path); 
    char *rest = filepath;

	char *ptr = strtok_r(rest, "/", &rest);
    if(ptr == NULL){
        return -1;
    }

    int next_inode = check_file_cp(ptr, rest, 1);

    if(next_inode == 1){
        return -1;
    }
    
    ptr = strtok_r(rest, "/", &rest);

    while(ptr != NULL){
    
        next_inode = check_file_cp(ptr, rest, next_inode - 1);
        if(next_inode == -1){
            return -1;
        }
    
        ptr = strtok_r(rest, "/", &rest); // Get next strtok_r

    }
    return next_inode;

}

int check_file_cp(char *ptr, char *rest, int inode_num){
    
    struct ext2_group_desc *gdt = (struct ext2_group_desc *)(disk + EXT2_BLOCK_SIZE * 2);
    struct ext2_inode *inode_tbl = (struct ext2_inode *)(disk + EXT2_BLOCK_SIZE * gdt->bg_inode_table);

    pthread_mutex_lock(&inode_table_lock);
    for(int j = 0; j < 13; j++){
        
        // e.g. /foo/bar/blah -- handle cases for foo and bar (non-directory error)
        if((inode_tbl[inode_num].i_mode & 0x4000) && (inode_tbl[inode_num].i_block[j] != 0) && (strlen(rest) != 0)) {
            printf("got into IF for non last directory\n");

            struct ext2_dir_entry *entry = (struct ext2_dir_entry *)(disk + EXT2_BLOCK_SIZE * inode_tbl[inode_num].i_block[j]);
            unsigned short offset = entry->rec_len;
            
            while(offset < EXT2_BLOCK_SIZE){ // Run linked list

                entry = (struct ext2_dir_entry *)(disk + EXT2_BLOCK_SIZE * inode_tbl[inode_num].i_block[j] + offset);
                char name[entry->name_len];
                strncpy(name,entry->name, entry->name_len);
                name[entry->name_len] = '\0';

                if (strcmp(ptr, name) == 0) {
                    printf("Found File Success, go into next inode\n");
                    if(entry->file_type == 1){
                        pthread_mutex_unlock(&inode_table_lock);
                        return -1; // Case 3 and 4b
                    }
                    pthread_mutex_unlock(&inode_table_lock);
                    return entry->inode;
                } else {
                    offset = offset + entry->rec_len;
                }
            }
            pthread_mutex_unlock(&inode_table_lock);
            return -1;

        }

        // Reach here if path is last e.g. /foo/bar/blah <-- points to blah
        else if ((inode_tbl[inode_num].i_block[j] != 0) && (strlen(rest) == 0)) {
            printf("got into IF for LAST directory\n");

            struct ext2_dir_entry *entry = (struct ext2_dir_entry *)(disk + EXT2_BLOCK_SIZE * inode_tbl[inode_num].i_block[j]);
            unsigned short offset = entry->rec_len;
            
            while(offset < EXT2_BLOCK_SIZE){ // Run linked list

                entry = (struct ext2_dir_entry *)(disk + EXT2_BLOCK_SIZE * inode_tbl[inode_num].i_block[j] + offset);
                char name[entry->name_len];
                strncpy(name,entry->name, entry->name_len);
                name[entry->name_len] = '\0';

                if (strcmp(ptr, name) == 0) {
                    printf("Found blah in Last directory, handle cases\n");
                    // Case 2, 3, 4, 5, /blah exists and is either a file or directory
                    pthread_mutex_unlock(&inode_table_lock);
                    return entry->inode; 
                } else if (offset + entry->rec_len == EXT2_BLOCK_SIZE) {
                    // Case 1: where /blah does not exist, return /blah's parent inode
                    entry = (struct ext2_dir_entry *)(disk + EXT2_BLOCK_SIZE * inode_tbl[inode_num].i_block[j]);
                    pthread_mutex_unlock(&inode_table_lock);
                    return (entry->inode) * -1; // Returns parent inode (as a negative)
                } else {
                    offset = offset + entry->rec_len;
                }
            }
            pthread_mutex_unlock(&inode_table_lock);
            return -1; // Not found in linked list 
        }
    }
    pthread_mutex_unlock(&inode_table_lock);
    return 0; // No space left in bitmap (e.g. 00000000)

}

int check_filepath_mkdir(const char *path){

    char *filepath = strdup(path); 
    char *rest = filepath;

	char *ptr = strtok_r(rest, "/", &rest);
    if(ptr == NULL){
        return -1;
    }
    int next_inode = check_file_mkdir(ptr, rest, 1);

    if(next_inode == -1) {
        printf("here\n");
        return -1;
    } 
    
    ptr = strtok_r(rest, "/", &rest); // Get next strtok_r

    while(ptr != NULL) { // Goes through each directory in Path
        printf("ptr %s\n", ptr);
        printf("next inode %d\n", next_inode);
        next_inode = check_file_mkdir(ptr, rest, next_inode - 1);
        if (next_inode == -1) {
            return -1;
        }
        if (next_inode == -2){
            return -2;
        }
        printf("new next inode%d\n", next_inode - 1);  
        ptr = strtok_r(rest, "/", &rest); // Get next strtok_r

    }
    return next_inode; // or do we return just next_inode?
     
}

int check_file_mkdir(char *ptr, char *rest, int inode_num){

    struct ext2_group_desc *gdt = (struct ext2_group_desc *)(disk + EXT2_BLOCK_SIZE * 2);
    struct ext2_inode *inode_tbl = (struct ext2_inode *)(disk + EXT2_BLOCK_SIZE * gdt->bg_inode_table);
    
    pthread_mutex_lock(&inode_table_lock);
    for(int j = 0; j < 13; j++){
        
        if((inode_tbl[inode_num].i_mode & 0x4000) && (inode_tbl[inode_num].i_block[j] != 0)) {
            printf("got into if statement -- helper\n");

            struct ext2_dir_entry *entry = (struct ext2_dir_entry *)(disk + EXT2_BLOCK_SIZE * inode_tbl[inode_num].i_block[j]);
            unsigned short offset = entry->rec_len;
            
            while(offset < EXT2_BLOCK_SIZE){ // Run linked list
                printf("got into while in linked list \n"); 
                entry = (struct ext2_dir_entry *)(disk + EXT2_BLOCK_SIZE * inode_tbl[inode_num].i_block[j] + offset);
                char name[entry->name_len];
                strncpy(name,entry->name, entry->name_len);
                name[entry->name_len] = '\0';


                printf("offset: %d\n", offset);
                printf("rec len: %d\n", entry->rec_len);
                printf("ptr in checkfile %s\n", ptr);
                printf("name %s\n", name);


                if (strcmp(ptr, name) == 0) {
                    printf("Found File Success, go into next inode\n");
                    if((entry->file_type == 2) && (strlen(rest) == 0)){
                        pthread_mutex_unlock(&inode_table_lock);
                        return -2;    // Case 2
                    }else if(entry->file_type == 1){
                        pthread_mutex_unlock(&inode_table_lock);
                        return -1; // Case 3 and 4b
                    }
                    pthread_mutex_unlock(&inode_table_lock);
                    return entry->inode;
                } else {

                    offset = offset + entry->rec_len;
                }
            }
        
            if(strlen(rest) == 0){
                pthread_mutex_unlock(&inode_table_lock);
                return inode_num; 
            
            }

        }

    }
    pthread_mutex_unlock(&inode_table_lock);
    return -1; // Case 4a
}

int check_filepath_rm(const char *path){

    char *filepath = strdup(path); 
    char *rest = filepath;

	char *ptr = strtok_r(rest, "/", &rest);
    if(ptr == NULL){
        return -1;
    }
    int next_inode = check_file_rm(ptr, rest, 1);

    if(next_inode == 1){
        return -1;
    }

    if(next_inode == -2){
        return -2;
    }
    
    ptr = strtok_r(rest, "/", &rest);

    while(ptr != NULL){
    
        next_inode = check_file_rm(ptr, rest, next_inode - 1);
        if(next_inode == -1){
            return -1;
        }

        if(next_inode == -2){
            return -2;
        }
        ptr = strtok_r(rest, "/", &rest); // Get next strtok_r

    }
    return next_inode;

}

int check_file_rm(char *ptr, char *rest, int inode_num){
    
    struct ext2_group_desc *gdt = (struct ext2_group_desc *)(disk + EXT2_BLOCK_SIZE * 2);
    struct ext2_inode *inode_tbl = (struct ext2_inode *)(disk + EXT2_BLOCK_SIZE * gdt->bg_inode_table);

    pthread_mutex_lock(&inode_table_lock);
    for(int j = 0; j < 13; j++){
        
        // e.g. /foo/bar/blah -- handle cases for foo and bar (non-directory error)
        if((inode_tbl[inode_num].i_mode & 0x4000) && (inode_tbl[inode_num].i_block[j] != 0) && (strlen(rest) != 0)) {
            printf("got into IF for non last directory\n");

            struct ext2_dir_entry *entry = (struct ext2_dir_entry *)(disk + EXT2_BLOCK_SIZE * inode_tbl[inode_num].i_block[j]);
            unsigned short offset = 0;
            
            while(offset < EXT2_BLOCK_SIZE){ // Run linked list

                entry = (struct ext2_dir_entry *)(disk + EXT2_BLOCK_SIZE * inode_tbl[inode_num].i_block[j] + offset);
                char name[entry->name_len];
                strncpy(name,entry->name, entry->name_len);
                name[entry->name_len] = '\0';

                if (strcmp(ptr, name) == 0) {
                    printf("Found File Success, go into next inode\n");
                    if(entry->file_type == 1){
                        pthread_mutex_unlock(&inode_table_lock);
                        return -1; // Case 3 and 4b
                    }
                    pthread_mutex_unlock(&inode_table_lock);
                    return entry->inode;
                } else {
                    offset = offset + entry->rec_len;
                }
            }
            pthread_mutex_unlock(&inode_table_lock);
            return -1;

        }

        // Reach here if path is last e.g. /foo/bar/blah <-- points to blah
        else if ((inode_tbl[inode_num].i_block[j] != 0) && (strlen(rest) == 0)) {
            printf("got into IF for LAST directory\n");

            struct ext2_dir_entry *entry = (struct ext2_dir_entry *)(disk + EXT2_BLOCK_SIZE * inode_tbl[inode_num].i_block[j]);
            unsigned short offset = 0;
            
            while(offset < EXT2_BLOCK_SIZE){ // Run linked list

                int prev_rec_len = entry->rec_len;
                printf("prev rec len is %d for inode %d\n", prev_rec_len, entry->inode);
                printf("offset is: %d\n", offset);
                entry = (struct ext2_dir_entry *)(disk + EXT2_BLOCK_SIZE * inode_tbl[inode_num].i_block[j] + offset);
                char name[entry->name_len];
                strncpy(name,entry->name, entry->name_len);
                name[entry->name_len] = '\0';

                if (strcmp(ptr, name) == 0) {
                    if (entry->file_type == 2) { // Case 2: Last path thing is a directory
                        pthread_mutex_unlock(&inode_table_lock);
                        return -2; 
                    }
                    printf("entry if we found it\n");
                    pthread_mutex_unlock(&inode_table_lock);
                    int err = remove_entry(inode_num, j, offset, prev_rec_len); // call helper to remove inode
                    if (err == 1) {
                        return -1; // not success 
                    } else {
                        return 0; // success 
                    }
                } else {
                    offset = offset + entry->rec_len;
                }
            }
            pthread_mutex_unlock(&inode_table_lock);
            return -1; // Not found in linked list 
        }
    }
    pthread_mutex_unlock(&inode_table_lock);
    return 0; // No space left in bitmap -- rare case (e.g. 00000000)

}

int check_filepath_hl(const char *path, int path_type){

    char *filepath = strdup(path); 
    char *rest = filepath;

	char *ptr = strtok_r(rest, "/", &rest);
    if(ptr == NULL){
        return -1;
    }
    int next_inode = check_file_hl(ptr, rest, 1, path_type);

    if(next_inode == 1){
        return -1;
    } else if(next_inode == -2) {
        return -2;
    } else if (next_inode == -3) {
        return -3;
    }
    
    ptr = strtok_r(rest, "/", &rest);

    while(ptr != NULL){
    
        next_inode = check_file_hl(ptr, rest, next_inode - 1, path_type);
        printf("next inode is: %d\n", next_inode);
        if(next_inode == -1){
            return -1;
        } else if(next_inode == -2) {
            return -2;
        } else if (next_inode == -3) {
            return -3;
        }
        ptr = strtok_r(rest, "/", &rest); // Get next strtok_r

    }
    return next_inode;

}

int check_file_hl(char *ptr, char *rest, int inode_num, int path_type){
    
    struct ext2_group_desc *gdt = (struct ext2_group_desc *)(disk + EXT2_BLOCK_SIZE * 2);
    struct ext2_inode *inode_tbl = (struct ext2_inode *)(disk + EXT2_BLOCK_SIZE * gdt->bg_inode_table);

    pthread_mutex_lock(&inode_table_lock);
    for(int j = 0; j < 13; j++){
        
        // e.g. /foo/bar/blah -- handle cases for foo and bar (non-directory error)
        if((inode_tbl[inode_num].i_mode & 0x4000) && (inode_tbl[inode_num].i_block[j] != 0) && (strlen(rest) != 0)) {
            printf("got into IF for non last directory\n");

            struct ext2_dir_entry *entry = (struct ext2_dir_entry *)(disk + EXT2_BLOCK_SIZE * inode_tbl[inode_num].i_block[j]);
            unsigned short offset = entry->rec_len;
            
            while(offset < EXT2_BLOCK_SIZE){ // Run linked list

                entry = (struct ext2_dir_entry *)(disk + EXT2_BLOCK_SIZE * inode_tbl[inode_num].i_block[j] + offset);
                char name[entry->name_len];
                strncpy(name,entry->name, entry->name_len);
                name[entry->name_len] = '\0';

                if (strcmp(ptr, name) == 0) {
                    printf("Found File Success, go into next inode\n");
                    if(entry->file_type == 1){
                        pthread_mutex_unlock(&inode_table_lock);
                        return -1;
                    }
                    pthread_mutex_unlock(&inode_table_lock);
                    return entry->inode;
                } else {
                    offset = offset + entry->rec_len;
                }
            }
            pthread_mutex_unlock(&inode_table_lock);
            return -1;
        }

        // Reach here if path is last e.g. /foo/bar/blah <-- points to blah
        else if ((inode_tbl[inode_num].i_block[j] != 0) && (strlen(rest) == 0)) {
            printf("got into IF for LAST directory\n");

            struct ext2_dir_entry *entry = (struct ext2_dir_entry *)(disk + EXT2_BLOCK_SIZE * inode_tbl[inode_num].i_block[j]);
            unsigned short offset = entry->rec_len;
            
            while(offset < EXT2_BLOCK_SIZE){ // Run linked list

                entry = (struct ext2_dir_entry *)(disk + EXT2_BLOCK_SIZE * inode_tbl[inode_num].i_block[j] + offset);
                char name[entry->name_len];
                strncpy(name,entry->name, entry->name_len);
                name[entry->name_len] = '\0';

                if (strcmp(ptr, name) == 0) {
                    printf("matched ptr\n");
                    printf("file type: %d\n", entry->file_type);
                    printf("inode: %d\n", entry->inode);
                    printf("path type: %d\n", path_type);
                    if (entry->file_type == 2) { // If file is a directory
                        pthread_mutex_unlock(&inode_table_lock);
                        return -2; // Denotes EISDIR
                    } else if ((path_type == 1) && (entry->file_type == 1)) {
                        pthread_mutex_unlock(&inode_table_lock);
                        return -3; // Denotes EEXIST
                    }
                    pthread_mutex_unlock(&inode_table_lock);
                    return entry->inode;
                } else {
                    offset = offset + entry->rec_len;
                }
            }
            if (path_type == 1) {
                entry = (struct ext2_dir_entry *)(disk + EXT2_BLOCK_SIZE * inode_tbl[inode_num].i_block[j]);
                printf("in while else: %d\n", entry->inode);
                pthread_mutex_unlock(&inode_table_lock);
                return entry->inode; 
            } else {
                pthread_mutex_unlock(&inode_table_lock);
                return -1;
            }
        }
    }
    pthread_mutex_unlock(&inode_table_lock);
    return 0; // No space left in bitmap (e.g. 00000000)

}

int check_filepath_sl(const char *path){

    char *filepath = strdup(path); 
    char *rest = filepath;

	char *ptr = strtok_r(rest, "/", &rest);
    if(ptr == NULL){
        return -1;
    }
    int next_inode = check_file_sl(ptr, rest, 1);

    if(next_inode == 1){
        return -1;
    } else if(next_inode == -2) {
        return -2;
    } else if (next_inode == -3) {
        return -3;
    }
    
    ptr = strtok_r(rest, "/", &rest);

    while(ptr != NULL){
    
        next_inode = check_file_sl(ptr, rest, next_inode - 1);
        if(next_inode == -1){
            return -1;
        } else if(next_inode == -2) {
            return -2;
        } else if (next_inode == -3) {
            return -3;
        }
        ptr = strtok_r(rest, "/", &rest); // Get next strtok_r

    }
    return next_inode;

}

int check_file_sl(char *ptr, char *rest, int inode_num){
    
    struct ext2_group_desc *gdt = (struct ext2_group_desc *)(disk + EXT2_BLOCK_SIZE * 2);
    struct ext2_inode *inode_tbl = (struct ext2_inode *)(disk + EXT2_BLOCK_SIZE * gdt->bg_inode_table);

    pthread_mutex_lock(&inode_table_lock);
    for(int j = 0; j < 13; j++){
        
        // e.g. /foo/bar/blah -- handle cases for foo and bar (non-directory error)
        if((inode_tbl[inode_num].i_mode & 0x4000) && (inode_tbl[inode_num].i_block[j] != 0) && (strlen(rest) != 0)) {
            printf("got into IF for non last directory\n");

            struct ext2_dir_entry *entry = (struct ext2_dir_entry *)(disk + EXT2_BLOCK_SIZE * inode_tbl[inode_num].i_block[j]);
            unsigned short offset = entry->rec_len;
            
            while(offset < EXT2_BLOCK_SIZE){ // Run linked list

                entry = (struct ext2_dir_entry *)(disk + EXT2_BLOCK_SIZE * inode_tbl[inode_num].i_block[j] + offset);
                char name[entry->name_len];
                strncpy(name,entry->name, entry->name_len);
                name[entry->name_len] = '\0';

                if (strcmp(ptr, name) == 0) {
                    printf("Found File Success, go into next inode\n");
                    if(entry->file_type == 1){
                        pthread_mutex_unlock(&inode_table_lock);
                        return -1;
                    }
                    pthread_mutex_unlock(&inode_table_lock);
                    return entry->inode;
                } else {
                    offset = offset + entry->rec_len;
                }
            }
            pthread_mutex_unlock(&inode_table_lock);
            return -1;
        }

        // Reach here if path is last e.g. /foo/bar/blah <-- points to blah
        else if ((inode_tbl[inode_num].i_block[j] != 0) && (strlen(rest) == 0)) {
            printf("got into IF for LAST directory\n");

            struct ext2_dir_entry *entry = (struct ext2_dir_entry *)(disk + EXT2_BLOCK_SIZE * inode_tbl[inode_num].i_block[j]);
            unsigned short offset = entry->rec_len;
            
            while(offset < EXT2_BLOCK_SIZE){ // Run linked list

                entry = (struct ext2_dir_entry *)(disk + EXT2_BLOCK_SIZE * inode_tbl[inode_num].i_block[j] + offset);
                char name[entry->name_len];
                strncpy(name,entry->name, entry->name_len);
                name[entry->name_len] = '\0';

                if (strcmp(ptr, name) == 0) {
                    printf("matched ptr\n");
                    printf("file type: %d\n", entry->file_type);
                    printf("inode: %d\n", entry->inode);
                    if (entry->file_type == 2) { // If file is a directory
                        pthread_mutex_unlock(&inode_table_lock);
                        return -2; // Denotes EISDIR
                    } else {
                        pthread_mutex_unlock(&inode_table_lock);
                        return -3; // Denotes EEXIST
                    }
                } else {
                    offset = offset + entry->rec_len;
                }
            }
            entry = (struct ext2_dir_entry *)(disk + EXT2_BLOCK_SIZE * inode_tbl[inode_num].i_block[j]);
            printf("foo's parent inode: %d\n", entry->inode);
            pthread_mutex_unlock(&inode_table_lock);
            return entry->inode; // Return parents inode
        }
    }
    pthread_mutex_unlock(&inode_table_lock);
    return -1; // No space left in bitmap (e.g. 00000000)

}

// Helper for rm operation, given a previous entry, removes the next entry in the linked list
int remove_entry(int parent_inode, int block_index, int offset, int prev_rec_len) {

    pthread_mutex_lock(&inode_table_lock);
    struct ext2_group_desc *gdt = (struct ext2_group_desc *)(disk + EXT2_BLOCK_SIZE * 2);
    struct ext2_inode *inode_tbl = (struct ext2_inode *)(disk + EXT2_BLOCK_SIZE * gdt->bg_inode_table);
    printf("offset, in remove_entry: %d\n", offset);
    printf("prev rec len in remove entry %d\n", prev_rec_len);
    struct ext2_dir_entry *prev_entry = (struct ext2_dir_entry *)(disk + EXT2_BLOCK_SIZE * inode_tbl[parent_inode].i_block[block_index] + offset);
    printf("entry: %d\n", prev_entry->inode);
    printf("rec len %d for entry inode %d\n", prev_entry->rec_len, prev_entry->inode);
    prev_entry = (struct ext2_dir_entry *)(disk + EXT2_BLOCK_SIZE * inode_tbl[parent_inode].i_block[block_index] + (offset - prev_rec_len));
    printf("entry: %d\n", prev_entry->inode);
    struct ext2_dir_entry *entry_to_rm = (struct ext2_dir_entry *)(disk + EXT2_BLOCK_SIZE * inode_tbl[parent_inode].i_block[block_index] + offset);
    printf("entry: %d\n", entry_to_rm->inode);

    // --- Deletion Process ---
    prev_entry->rec_len = prev_entry->rec_len + entry_to_rm->rec_len;

    // Adjust prev_entry rec_len to skip over the entry_to_rm

    struct ext2_inode *inode_to_delete  = &(inode_tbl[entry_to_rm->inode - 1]);
    if (inode_to_delete->i_links_count > 0) {
       inode_to_delete->i_links_count = inode_to_delete->i_links_count - 1;
    }

    if (inode_to_delete->i_links_count != 0) {
       // Don't free inode (do nothing ... for now)
    } else {
       // Does not link to any links? so free inode
       pthread_mutex_unlock(&inode_table_lock);
       free_blocks(entry_to_rm->inode, 1);
       printf("removing entry inode at: %d\n", entry_to_rm->inode);
       deallocate_inode(entry_to_rm->inode);
       
       inode_to_delete->i_size = 0;
       inode_to_delete->i_dtime = time(NULL);   
    }

    pthread_mutex_unlock(&inode_table_lock);
    return 0;
}

// Function is treated as a global helper for creating a new directory
int create_new_directory_or_file(int parent_inode, char *name,	unsigned short mode, int node_type){

    struct ext2_group_desc *gdt = (struct ext2_group_desc *)(disk + EXT2_BLOCK_SIZE * 2);
    struct ext2_inode *inode_tbl = (struct ext2_inode *)(disk + EXT2_BLOCK_SIZE * gdt->bg_inode_table);

    printf("inode in create_new_dir %d\n", parent_inode);

    struct ext2_dir_entry *new_item = check_space(parent_inode, strlen(name), node_type);
    if(new_item == NULL){
        return ENOSPC * -1;
    }

    // Allocate new inode for new directory and find first free inode in bitmap
    int new_inode_idx = allocate_inode();
    if(new_inode_idx == -1){
        return ENOSPC * -1;
    }

    printf("inode here is  %d\n", new_inode_idx);

    struct ext2_inode new_inode;

    if(mode == EXT2_FT_SYMLINK){
        new_inode.i_mode = EXT2_S_IFLNK;
    } else {
        new_inode.i_mode = mode;
    }
    new_inode.i_uid = 0;
    if(mode == EXT2_S_IFDIR){
        new_inode.i_size = EXT2_BLOCK_SIZE;
    } else{
        new_inode.i_size = 0;
    }
    new_inode.i_ctime = time(NULL);
    new_inode.i_dtime = 0;
    new_inode.i_gid = 0;
    if(mode == EXT2_S_IFDIR){
        new_inode.i_links_count = 2;
    }else{
        new_inode.i_links_count = 1;
    }
    new_inode.i_blocks = 2;
    new_inode.osd1 = 0;
    
    int block = allocate_block();
    if(block == -1){
        return ENOSPC * -1;
    }
    printf("new block %d\n", block);

    for (int i = 0; i < 15; i++) {
        new_inode.i_block[i] = 0;
    }
    new_inode.i_block[0] = block;
    new_inode.i_generation = 0;
    new_inode.i_file_acl = 0;
    new_inode.i_dir_acl = 0;
    new_inode.i_faddr = 0;
    new_inode.extra[0] = 0;
    new_inode.extra[1] = 0;
    new_inode.extra[2] = 0;

    pthread_mutex_lock(&inode_table_lock);
    inode_tbl[new_inode_idx - 1] = new_inode;
    pthread_mutex_unlock(&inode_table_lock);

    if(mode == EXT2_S_IFDIR){ // Create new Directory 

        // Need to set values for new_directory
        new_item->file_type = EXT2_FT_DIR;
        strcpy(new_item->name, name);
        new_item->name_len = strlen(name);
        new_item->inode = new_inode_idx;

        struct ext2_dir_entry *temp_entry = (struct ext2_dir_entry *)(disk + EXT2_BLOCK_SIZE * (block));

        temp_entry->inode = new_inode_idx;
        temp_entry->file_type = EXT2_FT_DIR;
        strcpy(temp_entry->name, ".");
        temp_entry->name_len = 1;
        temp_entry->rec_len = 12;

        temp_entry = (struct ext2_dir_entry *)(disk + (EXT2_BLOCK_SIZE * (block)) + 12);
        temp_entry->inode =  parent_inode + 1;
        temp_entry->file_type = EXT2_FT_DIR;
        strcpy(temp_entry->name, "..");
        temp_entry->name_len = 2;
        temp_entry->rec_len = EXT2_BLOCK_SIZE - 12;
        struct ext2_inode *node = &(inode_tbl[parent_inode]);
        node->i_links_count++;

    }else if(mode == EXT2_S_IFREG) { // Create new File
    
        // Need to set values for new_directory
        new_item->file_type = EXT2_FT_REG_FILE;
        strcpy(new_item->name, name);
        new_item->name_len = strlen(name);
        new_item->inode = new_inode_idx;
        
    } else if(new_inode.i_mode == EXT2_S_IFLNK){
        
        new_item->inode = new_inode_idx;
        new_item->file_type = EXT2_FT_SYMLINK;
        strcpy(new_item->name, name);
        new_item->name_len = strlen(name);
        
    }

    
    return new_inode_idx;
}

// Function is treated as a global helper for creating a new directory
int create_new_link(int parent_inode, char *name, unsigned short mode, int source_inode){

    struct ext2_group_desc *gdt = (struct ext2_group_desc *)(disk + EXT2_BLOCK_SIZE * 2);
    struct ext2_inode *inode_tbl = (struct ext2_inode *)(disk + EXT2_BLOCK_SIZE * gdt->bg_inode_table);

    printf("parent inode in create_new_dir %d\n", parent_inode);
    printf("source inode in create_new_dir %d\n", source_inode);
    
    if(mode == EXT2_FT_REG_FILE){
        struct ext2_dir_entry *new_entry = check_space(parent_inode, strlen(name), 1);
        if(new_entry == NULL){
            return ENOSPC * -1;
        }
        new_entry->file_type = EXT2_FT_REG_FILE;
        strcpy(new_entry->name, name);
        new_entry->name_len = strlen(name);
        new_entry->inode = source_inode;

    } else if(mode == EXT2_FT_SYMLINK){
        int new_inode_idx = create_new_directory_or_file(parent_inode, name, mode, 1);
        if(new_inode_idx == (ENOSPC * -1)){
            return ENOSPC * -1;
        }
        return new_inode_idx;

    }

    pthread_mutex_lock(&inode_table_lock);
    struct ext2_inode *inode = &(inode_tbl[source_inode - 1]);
    inode->i_links_count += 1;
    pthread_mutex_unlock(&inode_table_lock);
    return 0;
    
}

// Function treated as a global for checking space
struct ext2_dir_entry *check_space(int inode_num, int dir_len, int node_type){

    struct ext2_group_desc *gdt = (struct ext2_group_desc *)(disk + EXT2_BLOCK_SIZE * 2);
    struct ext2_inode *inode_tbl = (struct ext2_inode *)(disk + EXT2_BLOCK_SIZE * gdt->bg_inode_table);
    printf("inode in check_space %d\n", inode_num);

    pthread_mutex_lock(&inode_table_lock);
    for(int j = 0; j < 13; j++){

        // If you are given a parent inode (case 1), then minus one this right here: inode_num -= 1;
        // but if you are doing non parent or direct (case 3, 4 or 5) then don't minus one 
        // or else it will make a new directory in a level above the desired directory

        if (node_type == 1) { // Let node_type: 1 denote inode that is a parent entry
            inode_num -= 1;
        }

        if((inode_tbl[inode_num].i_block[j] != 0)){

            struct ext2_dir_entry *entry = (struct ext2_dir_entry *)(disk + EXT2_BLOCK_SIZE * inode_tbl[inode_num].i_block[j]);
            printf("inode_num %d\n", inode_num);             
            printf("inode: %d reclen: %d namelen: %d\n", entry->inode, entry->rec_len, entry->name_len);

            unsigned short offset = entry->rec_len;

            while(offset != EXT2_BLOCK_SIZE){ // Run linked list
                printf("offset checkspace %d\n", offset);
                entry = (struct ext2_dir_entry *)(disk + EXT2_BLOCK_SIZE * inode_tbl[inode_num].i_block[j] + offset);
                printf("inode: %d reclen: %d namelen: %d\n", entry->inode, entry->rec_len, entry->name_len);


                if(offset + entry->rec_len == EXT2_BLOCK_SIZE){ // Reach end of linked list 
                    printf("got into end of linked list if statement\n");
                    if (offset + 8 + entry->name_len + 8 + dir_len <= 1024 -1) { // If there is enough space in block
                        printf("reached end of linked list first if statement\n");
                        printf("Got into inode: %d reclen: %d namelen: %d\n", entry->inode, entry->rec_len, entry->name_len);
                        entry->rec_len = 8 + entry->name_len;
                        while(entry->rec_len % 4){
                            entry->rec_len++;
                        }
                        struct ext2_dir_entry *new_entry = (struct ext2_dir_entry *)(disk + EXT2_BLOCK_SIZE * inode_tbl[inode_num].i_block[j] + entry->rec_len + offset);
                        new_entry->rec_len = EXT2_BLOCK_SIZE - offset - entry->rec_len;
                        printf("inode: %d reclen: %d namelen: %d\n", new_entry->inode, new_entry->rec_len, new_entry->name_len);
                        pthread_mutex_unlock(&inode_table_lock);
                        return new_entry;

                    } else {
                        printf("allocate block here TODO\n");
                        int block_num = allocate_block();
                        inode_tbl[inode_num].i_block[j+1] = block_num;
                        entry = (struct ext2_dir_entry *)(disk + EXT2_BLOCK_SIZE * inode_tbl[inode_num].i_block[j+1]);
                        pthread_mutex_unlock(&inode_table_lock);
                        return entry;
                    }
                }
                offset = offset + entry->rec_len;
            }
        }
    }
    pthread_mutex_unlock(&inode_table_lock);
    return NULL;
}

// A path_walker for cp that operates on cases 4, 5 and 6
int existing_file_walker(int inode_num, char *file_to_copy, FILE* sourcefile) {
    // Loop through linked list looking for if doh.txt exists

    pthread_mutex_lock(&inode_table_lock);
    for(int j = 0; j < 13; j++){

        struct ext2_group_desc *gdt = (struct ext2_group_desc *)(disk + EXT2_BLOCK_SIZE * 2);
        struct ext2_inode *inode_tbl = (struct ext2_inode *)(disk + EXT2_BLOCK_SIZE * gdt->bg_inode_table);
        
        if((inode_tbl[inode_num - 1].i_block[j] != 0)){

            struct ext2_dir_entry *entry = (struct ext2_dir_entry *)(disk + EXT2_BLOCK_SIZE * inode_tbl[inode_num - 1].i_block[j]);
            unsigned short offset = entry->rec_len;

            while(offset != EXT2_BLOCK_SIZE){ // Run linked list
                printf("running in while loop\n");
                entry = (struct ext2_dir_entry *)(disk + EXT2_BLOCK_SIZE * inode_tbl[inode_num - 1].i_block[j] + offset);
                char name[entry->name_len];
                strncpy(name,entry->name, entry->name_len);
                name[entry->name_len] = '\0';
                printf("name %s\n", name);

                if (strcmp(file_to_copy, name) == 0) { // Case 6: doh.txt exists under /blah, overwrite file
                    printf("Overwrite the doh file: Case 6\n");
                    // Clear/free first, THEN insert copy helper here

                    pthread_mutex_unlock(&inode_table_lock);
                    int block_index = free_blocks(entry->inode, 0);

                    if (block_index == ENOSPC * -1) {
                        return ENOSPC;
                    } else {
                        file_copier(sourcefile, entry->inode, 0, NULL);
                        return 0;
                    }
                } else {
                    offset = offset + entry->rec_len;
                }
            }

            // Case 4, 5: doh.txt does not exists under /blah, create file, then overwrite it 
            pthread_mutex_unlock(&inode_table_lock);
            int new_inode_idx = create_new_directory_or_file(inode_num, file_to_copy, EXT2_S_IFREG, 1); 
            printf("new_inode_idx is %d\n", new_inode_idx);
            
            if(new_inode_idx == -ENOSPC){
                return ENOSPC;
            }

            // Then overwrite it 
            printf("Overwrite the doh file: Case 4 & 5\n");
            file_copier(sourcefile, new_inode_idx, j, NULL);
            return 0;
        }

    }
    pthread_mutex_unlock(&inode_table_lock);
    return 0;
}

// Given a DIRECT Inode number, determine if that inode block is a file or a directory, operate based on those 
int inode_type_checker(int inode_num){

    struct ext2_group_desc *gdt = (struct ext2_group_desc *)(disk + EXT2_BLOCK_SIZE * 2);
    struct ext2_inode *inode_tbl = (struct ext2_inode *)(disk + EXT2_BLOCK_SIZE * gdt->bg_inode_table);
    
    pthread_mutex_lock(&inode_table_lock);
    // If inode block is a file -- return 0; else 1 for directory
    if(inode_tbl[inode_num-1].i_mode & 0x8000){
        pthread_mutex_unlock(&inode_table_lock);
        return 0;
    } else {
        pthread_mutex_unlock(&inode_table_lock);
        return 1;
    }
}

// Function that actually copies 'content' over 
int file_copier(FILE* input_file, int inode_num, int block_index, char *source) {
    if (source == NULL) {   
        int size_read;
        int blocks_read = 0;
        char buffer[EXT2_BLOCK_SIZE];
        printf("Got before while loop\n");
        while((blocks_read < 12) && ((size_read = fread(buffer, 1, EXT2_BLOCK_SIZE, input_file)) > 0)) {
            printf("size read: %d\n", size_read);
            blocks_read += 1;
            printf("blocks read: %d\n", blocks_read);
            printf("inode to write to %d at block idx %d\n", inode_num, block_index);
            write_to_inode(buffer, inode_num, block_index, size_read); 
            //write to inode at j
        }
        printf("Got after while loop\n");
        return 0;

    }else {
        char buffer[EXT2_BLOCK_SIZE];
        strncpy(buffer,source,strlen(source));
        write_to_inode(buffer, inode_num, block_index,strlen(source));
        return 0;
    }
}

// Using memcpy, writes to block
int write_to_inode(char *buffer, int inode_num, int block_index, int size_read){

    
    struct ext2_group_desc *gdt = (struct ext2_group_desc *)(disk + EXT2_BLOCK_SIZE * 2);
    struct ext2_inode *inode_tbl = (struct ext2_inode *)(disk + EXT2_BLOCK_SIZE * gdt->bg_inode_table);
    pthread_mutex_lock(&inode_table_lock);

    unsigned int *size = &(inode_tbl[inode_num - 1].i_size);
    *size = size_read;
    
    printf("Reached right before memcopy\n");
    printf("block number is %d\n", inode_tbl[inode_num - 1].i_block[block_index]);
    memcpy((void *)(disk + EXT2_BLOCK_SIZE * inode_tbl[inode_num - 1].i_block[block_index]), buffer, size_read);

    pthread_mutex_unlock(&inode_table_lock);
    return 0;
}

// Operation 0 denotes cp, 1 denotes rm
int free_blocks(int inode_num, int operation){

    struct ext2_group_desc *gdt = (struct ext2_group_desc *)(disk + EXT2_BLOCK_SIZE * 2);
    struct ext2_inode *inode_tbl = (struct ext2_inode *)(disk + EXT2_BLOCK_SIZE * gdt->bg_inode_table);
    
    pthread_mutex_lock(&inode_table_lock);
    struct ext2_inode *inode_to_clear = &(inode_tbl[inode_num - 1]);

    for(int i = 0; i < 13; i++){
        if(inode_to_clear->i_block[i] != 0){
            unsigned int block_to_deallocate = inode_to_clear->i_block[i];
            //memset to \0
            memset((void *)(disk + EXT2_BLOCK_SIZE * inode_tbl[inode_num - 1].i_block[i]), '\0', EXT2_BLOCK_SIZE);
            // Flip bit
            deallocate_block(block_to_deallocate + 1);
            inode_to_clear->i_block[i] = 0;
        }
    }
    pthread_mutex_unlock(&inode_table_lock);
    if(operation == 0){    // If CP calls free_blocks
        int block = allocate_block();
        if(block == -1){
            return ENOSPC * -1;
        }
        
        inode_to_clear->i_block[0] = block;
        inode_to_clear->i_blocks = 2;
        inode_to_clear->i_size = 0;
    
        return block;
    }
    return 0;

}

// Bit flipper for a specific inode
void deallocate_inode(int inode_index) {
    inode_index--;
    pthread_mutex_lock(&sb_lock);
    pthread_mutex_lock(&gdt_lock);
    struct ext2_super_block *sb = (struct ext2_super_block *)(disk + EXT2_BLOCK_SIZE);
    struct ext2_group_desc *gdt = (struct ext2_group_desc *)(disk + EXT2_BLOCK_SIZE * 2);
    unsigned char *inode_bits = (unsigned char*)(disk + EXT2_BLOCK_SIZE * gdt->bg_inode_bitmap);

    printf("inside deallocate inode function, the inode_index: %d\n", inode_index);
    
    pthread_mutex_lock(&inode_bitmap_lock);
    inode_bits[inode_index / 8] = inode_bits[inode_index / 8] & ~(1 << inode_index % 8); 
    
    sb->s_free_inodes_count++;
    gdt->bg_free_inodes_count++; 

    pthread_mutex_unlock(&inode_bitmap_lock);

    pthread_mutex_unlock(&gdt_lock);
    pthread_mutex_unlock(&sb_lock);
}

// Bit flipper for a specific block
void deallocate_block(int block_index) {
    
    // TODO: fix this shit
    //block_index--;

    pthread_mutex_lock(&sb_lock);
    pthread_mutex_lock(&gdt_lock);
    struct ext2_super_block *sb = (struct ext2_super_block *)(disk + EXT2_BLOCK_SIZE);
    struct ext2_group_desc *gdt = (struct ext2_group_desc *)(disk + EXT2_BLOCK_SIZE * 2);
    unsigned char *block_bits = (unsigned char*)(disk + EXT2_BLOCK_SIZE * gdt->bg_block_bitmap);

    pthread_mutex_lock(&block_bitmap_lock);

    printf("inside deallocate block function, the inode_index: %d\n", block_index);

    //block_bits[block_index / 8] = block_bits[block_index / 8] & (0 << block_index % 8); 
    
    int pos = 0;
    for(int i = 0; i < sb->s_blocks_count/8; i++){
        for(int j = 0; j < 8; j++){
            if(pos == block_index){
                block_bits[block_index / 8] = block_bits[block_index / 8] & ~(1 << (block_index % 8));
                sb->s_free_blocks_count++;
                gdt->bg_free_blocks_count++;    
            }
        }
        pos++;
    }
    pthread_mutex_unlock(&gdt_lock);
    pthread_mutex_unlock(&sb_lock);
    pthread_mutex_unlock(&block_bitmap_lock);

   
    
}
// Function treated as global for allocating a block
int allocate_block() {
    
    pthread_mutex_lock(&sb_lock);
    pthread_mutex_lock(&gdt_lock);
    struct ext2_super_block *sb = (struct ext2_super_block *)(disk + EXT2_BLOCK_SIZE);
    struct ext2_group_desc *gdt = (struct ext2_group_desc *)(disk + EXT2_BLOCK_SIZE * 2);
    unsigned char *block_bits = (unsigned char*)(disk + EXT2_BLOCK_SIZE * gdt->bg_block_bitmap);
    unsigned char byte1;

    pthread_mutex_lock(&block_bitmap_lock);
    for (int i = 0; i < sb->s_blocks_count / 8; i++){
        byte1 = block_bits[i];
        for (int j = 0; j < 8; j++){
            
            if(!(byte1 & (1 << j))){
                block_bits[i] |= 1 << j;
                pthread_mutex_unlock(&block_bitmap_lock);
                sb->s_free_blocks_count--;
                gdt->bg_free_blocks_count--;
                
                pthread_mutex_unlock(&gdt_lock);
                pthread_mutex_unlock(&sb_lock);
                return i*8 + j + 1;
            }
        }
    }
    pthread_mutex_unlock(&block_bitmap_lock);
    pthread_mutex_unlock(&gdt_lock);
    pthread_mutex_unlock(&sb_lock);
    
    return -1;
}

// Function treatd as global for allocating an incode
int allocate_inode(){

    pthread_mutex_lock(&sb_lock);
    pthread_mutex_lock(&gdt_lock);
    struct ext2_super_block *sb = (struct ext2_super_block *)(disk + EXT2_BLOCK_SIZE);
    struct ext2_group_desc *gdt = (struct ext2_group_desc *)(disk + EXT2_BLOCK_SIZE * 2);
    unsigned char *inode_bits = (unsigned char*)(disk + EXT2_BLOCK_SIZE * gdt->bg_inode_bitmap);
    unsigned char byte2;

    pthread_mutex_lock(&inode_bitmap_lock);
    for (int i = 0; i < sb->s_inodes_count / 8; i++){
        byte2 = inode_bits[i];
        for (int j = 0; j < 8; j++){
            
            if(!(byte2 & (1 << j))){
                inode_bits[i] |= 1 << j;
                pthread_mutex_unlock(&inode_bitmap_lock);
                sb->s_free_inodes_count--;
                gdt->bg_free_inodes_count--;
                pthread_mutex_unlock(&gdt_lock);
                pthread_mutex_unlock(&sb_lock);
                return i*8 + j + 1;
            }
        }
    }
    pthread_mutex_unlock(&inode_bitmap_lock);
    pthread_mutex_unlock(&gdt_lock);
    pthread_mutex_unlock(&sb_lock);
    return -1;
}
