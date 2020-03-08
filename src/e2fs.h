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

#ifndef CSC369_E2FS_H
#define CSC369_E2FS_H

#include "ext2.h"
#include <string.h>
#include <stdio.h>


/**
 * Implement the helpers in e2fs.c
 * Approx: 88 lines
 */

 // Helpers for -- GLOBAL
unsigned char filetype_helper(unsigned char type);
int create_new_directory_or_file(int parent_inode, char *dir_to_create, unsigned short mode, int node_type);
int create_new_link(int parent_inode, char *dir_to_create, unsigned short mode, int source_inode);
struct ext2_dir_entry *check_space(int inode_num, int dir_len, int node_type);
int allocate_block();
int allocate_inode();
void deallocate_block(int block_index);
void deallocate_inode(int inode_index);
int free_blocks(int inode_num, int operation);

//  Helpers for Operations -- mkdir
int check_filepath_mkdir(const char *path);
int check_file_mkdir(char *ptr, char *rest,int inode);

// Helpers for Operations -- cp
int check_filepath_cp(const char *);
int check_file_cp(char *ptr, char *rest, int inode);
int inode_type_checker(int inode_num); // Note: the inode_num is direct
int existing_file_walker(int inode_num, char *file_to_copy, FILE* sourcefile); // Note: inode_num is direct
int file_copier(FILE* input_file, int inode_num, int block_index, char *source);
int write_to_inode(char *, int inode_num, int block_index, int size); 

// Helper for Operations -- rm
int check_filepath_rm(const char *);
int check_file_rm(char *ptr, char *rest, int inode);
int remove_entry(int parent_inode, int block_index, int offset, int prev_rec_len);

// Helper for Operations -- hl
int check_filepath_hl(const char *, int path_type);
int check_file_hl(char *ptr, char *rest, int inode, int path_type);
int create_new_link(int parent_inode, char *name, unsigned short mode, int source_inode);

// Helper for Operations -- sl
int check_filepath_sl(const char *);
int check_file_sl(char *ptr, char *rest, int inode);
int create_new_link(int parent_inode, char *name, unsigned short mode, int source_inode);


#endif