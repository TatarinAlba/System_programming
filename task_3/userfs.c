#include "userfs.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>


#include <stdio.h>


enum {
	BLOCK_SIZE = 512,
	MAX_FILE_SIZE = 1024 * 1024 * 100,
};

/** Global error code. Set from any function on any error. */
static enum ufs_error_code ufs_error_code = UFS_ERR_NO_ERR;

struct block {
	/** Block memory. */
	char *memory;
	/** How many bytes are occupied. */
	int occupied;
	/** Next block in the file. */
	struct block *next;
	/** Previous block in the file. */
	struct block *prev;

	/* PUT HERE OTHER MEMBERS */
};

struct file {
		/** Double-linked list of file blocks. */
		struct block *block_list;
		/**
	 * Last block in the list above for fast access to the end
	 * of file.
	 */
	struct block *last_block;
	/** How many file descriptors are opened on the file. */
	int refs;
	/** File name. */
	char *name;
	/** Files are stored in a double-linked list. */
	struct file *next;
	struct file *prev;

	/* Checking if current file was deleted 1 - yes. 0 - no.*/
	short int is_ghost;
	
	int block_count;
	int last_symbol_at;
};

/** List of all files. */
static struct file *file_list = NULL;

struct filedesc {
	struct file *file;
	struct block* curr_block;
	int block_p;
	int pos_p;
	/* PUT HERE OTHER MEMBERS */

};

/**
 * An array of file descriptors. When a file descriptor is
 * created, its pointer drops here. When a file descriptor is
 * closed, its place in this array is set to NULL and can be
 * taken by next ufs_open() call.
 */
static struct filedesc **file_descriptors = NULL;
static int file_descriptor_count = 0;
static int file_descriptor_capacity = 0;


/**
 * Function for creating file descriptor onto existing file
 * @param file Pointer to the file we want to 
 * @retval > 0 File descriptor to the given file
*/
int 
initialize_fd(struct file* file) 
{
	file->refs++;
	// initialize list of file descriptors
	if (file_descriptor_capacity == 0) {
		file_descriptor_capacity++;
		file_descriptors = malloc(sizeof(struct filedesc*));
	} else if (file_descriptor_capacity <= file_descriptor_count) {
		file_descriptor_capacity *= 2;
		file_descriptors = realloc(file_descriptors, file_descriptor_capacity * sizeof(struct filedesc*));
	}
	file_descriptors[file_descriptor_count] = calloc(sizeof(struct filedesc), 1);
	file_descriptors[file_descriptor_count++]->file = file;
	return file_descriptor_count;
}


/**
 * Function for creating file and giving file descriptor to the file
 * @param name Name for new file
 * @retval > 0 File descriptor to the new file
*/
int
create_file(const char* new_file_name)  
{
	struct file* new_file = calloc(sizeof(struct file), 1);
	size_t filename_size = strlen(new_file_name) + 1;
	new_file->name = malloc(sizeof(char) * filename_size);
	strcpy(new_file->name, new_file_name);
	int fd = initialize_fd(new_file);
	if (file_list == NULL) {
		// If no files in the list we creating new one and assigning it to the file_list pointer
		file_list = new_file;
	} else {
		// Else, we are traversing until we will not find NULL and then we will add it to the end
		struct file* last_file = file_list;
		while (last_file->next != NULL) {
			last_file = last_file->next;
		}
		last_file->next = new_file;
		new_file->prev = last_file;
	}
	return fd;
}


enum ufs_error_code
ufs_errno()
{
	return ufs_error_code;
}

int
ufs_open(const char *filename, int flags)
{
	struct file* curr_file = file_list;
	while (curr_file != NULL) {
		if (strcmp(curr_file->name, filename) == 0) {
			int fd = initialize_fd(curr_file);
			return fd;
		}
		curr_file = curr_file->next;
	}
	if (flags == UFS_CREATE) {
		return create_file(filename);
	} else {
		// If we did not find the file nad file creation flag was not specified 
		// we will throw an exception (-1 code)
		ufs_error_code = UFS_ERR_NO_FILE;
		return -1;
	}
}

ssize_t
ufs_write(int fd, const char *buf, size_t size)
{
	if (fd <= file_descriptor_count && fd > 0 && file_descriptors[fd - 1] != NULL) {
		struct filedesc* file_desc = file_descriptors[fd - 1];
		int new_block_created = 0;
		if (file_desc->block_p * BLOCK_SIZE + file_desc->pos_p + size <= MAX_FILE_SIZE) {
			if (file_desc->file->block_list == NULL) {
				file_desc->file->block_list = calloc(sizeof(struct block), 1);
				file_desc->file->block_list->memory = malloc(BLOCK_SIZE);
				file_desc->file->last_block = file_desc->file->block_list;
				new_block_created = 1;
			}
			if (file_desc->curr_block == NULL) {
				file_desc->curr_block = file_desc->file->block_list;
			}
			struct block* curr_block = file_desc->curr_block;
			size_t counter = size;
			int* pos_p = &file_desc->pos_p;
			int buff_p = 0; 
			while (counter > 0) {
				curr_block->memory[(*pos_p)++] = buf[buff_p++];
				counter--;
				if (*pos_p >= BLOCK_SIZE) {
					*pos_p = 0;
					if (curr_block->next == NULL) {
						new_block_created = 1;
						file_desc->file->block_count++;
						curr_block->occupied = BLOCK_SIZE;
						curr_block->next = calloc(sizeof(struct block), 1);
						curr_block->next->prev = curr_block;
						file_desc->file->last_block = curr_block->next;
						curr_block->next->memory = malloc(BLOCK_SIZE);
					} 
					file_desc->block_p++;
					file_desc->curr_block = curr_block->next;
					curr_block = curr_block->next;
				} 
			} 
			if (new_block_created || (file_desc->file->last_block == curr_block && file_desc->pos_p > file_desc->file->last_symbol_at)) {
				file_desc->file->last_symbol_at = *pos_p;
				file_desc->file->last_block->occupied = *pos_p;
			}
			return size;
		} else {
			ufs_error_code = UFS_ERR_NO_MEM;
			return -1;
		}
	}
	ufs_error_code = UFS_ERR_NO_FILE;
	return -1;
}

ssize_t
ufs_read(int fd, char *buf, size_t size)
{
	if (fd <= file_descriptor_count && fd > 0 && file_descriptors[fd - 1] != NULL) {
		struct filedesc* file_desc = file_descriptors[fd - 1];
		int buf_p = 0;
		int readb = 0;
		if (file_desc->curr_block == NULL) {
			file_desc->curr_block = file_desc->file->block_list;
		}
		while ((file_desc->file->block_count > file_desc->block_p || 
			file_desc->file->last_symbol_at > file_desc->pos_p) && (size - readb) > 0) {
				buf[buf_p++] = file_desc->curr_block->memory[file_desc->pos_p++];
				readb++;
				if (file_desc->pos_p >= BLOCK_SIZE) {
					if (file_desc->block_p + 1 > file_desc->file->block_count) {
						break;
					}
					file_desc->pos_p = 0;
					file_desc->block_p++;
					file_desc->curr_block = file_desc->curr_block->next;
				}
		}
		return readb;
	}
	ufs_error_code = UFS_ERR_NO_FILE;
	return -1;
}

int
ufs_close(int fd)
{
	if (fd <= file_descriptor_count && fd > 0 && file_descriptors[fd - 1] != NULL) {
		file_descriptors[fd - 1]->file->refs--;
		if (file_descriptors[fd - 1]->file->refs == 0 && file_descriptors[fd - 1]->file->is_ghost == 1) {
			struct block* block = file_descriptors[fd - 1]->file->block_list;
			while(block != NULL) {
				free(block->memory);
				block->prev = NULL;
				struct block* temp = block;
				block = block->next;
				free(temp);
			}
			free(file_descriptors[fd - 1]->file->name);
			free(file_descriptors[fd - 1]->file);
		} 
		free(file_descriptors[fd - 1]);
		file_descriptors[fd - 1] = NULL;
		return 0;
	}
	ufs_error_code = UFS_ERR_NO_FILE;
	return -1;
}

int
ufs_delete(const char *filename)
{
	for (struct file* curr_file = file_list; curr_file != NULL; curr_file = file_list->next) {
		if (strcmp(curr_file->name, filename) == 0) {
			struct file* prev = curr_file->prev;
			struct file* next = curr_file->next;
			if (prev != NULL && next != NULL) {
				prev->next = next;
				next->prev = prev;
			} else if (prev != NULL) {
				prev->next = NULL;
			} else if (next != NULL) {
				next->prev = NULL;
			}
			if (file_list == curr_file) {
				file_list = next;
			}
			curr_file->next = NULL;
			curr_file->prev = NULL;
			curr_file->is_ghost = 1;
			if (curr_file->refs == 0) {
				struct block* block = curr_file->block_list;
				while(block != NULL) {
					free(block->memory);
					struct block* temp = block;
					block = block->next;
					free(temp);
				}
				free(curr_file->name);
				free(curr_file);
			}
			return 0;
		}
	}
	ufs_error_code = UFS_ERR_NO_FILE;
	return -1;
}

void
ufs_destroy(void)
{
	for (struct file* curr_file = file_list; curr_file != NULL; curr_file = file_list) {
		ufs_delete(curr_file->name);
	}
	for (int i = 0; i < file_descriptor_capacity; i++) {
		if (file_descriptors[i] != NULL) {
			ufs_close(i + 1);
		}
	}
	free(file_descriptors);
}
