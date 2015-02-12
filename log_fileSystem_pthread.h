#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<math.h>
#include<inttypes.h>
#include<pthread.h>
#include<unistd.h>
#include<ctype.h>
/* Structure Definitions
 */
struct idirectory{
  int dot_dot;//id of parent
  struct direct *dinfo;//id and name of this directory
  struct idirectory *next;// To maintain link list of directory
  char *path;  // Absolute path of the directory
  int id;
};
struct ifile{
  int fdes;//file descriptor
  int did;//id of parent directory
  char *path; // Absolute path of the file
  int id;
  struct ifile *next; // To maintain link list of files
  uint64_t start_addr;//starting address
  uint64_t no_of_blks;//No. of blks occupied by the file
  int valid; // To indicate if file is in use or deleted
};
struct log{
  uint64_t max_blks;  // Maximum number of blocks to be allocated
  uint64_t last_addr; // last_addr that can be assigned
  uint64_t free_blks; // No of free blocks
  uint64_t blk_size;  // Size of each block
  uint64_t disk_capacity;  // Total disk capacity of the file system
};
typedef struct idirectory *IDIR;
typedef struct ifile *IFILE;
typedef struct log *LOG;
/* Function declarations 
 *
 * */

/*
 *Thread functions
 */
void *compact(void*);
void *read_file(void*);
void *write_file(void*);


void exit_procedure();
int assign_id();
int assign_fdes();
IDIR init_dir(IDIR);
void free_file(IFILE);
void make_dir(char*);
void change_dir(char*);
IDIR find_dir_by_path(char *path);
IFILE find_file_by_path(char *path);
char* get_absolute_path(char*);
IDIR find_parent(char*);
void get_input(void);
void RemoveSpaces(char*);
char* extract_parameters(char*);
char* extract_function(char*);
uint64_t find_actual_size(uint64_t size);
LOG init_log(uint64_t disk_capacity,uint64_t blk_size);
void delete_file(char *path);
