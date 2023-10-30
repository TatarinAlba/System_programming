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

	/* PUT HERE OTHER MEMBERS */
};

/** List of all files. */
static struct file *file_list = NULL;

struct filedesc {
	struct file *file;

	/* PUT HERE OTHER MEMBERS */
	int file_desc_value;
};

/**
 * An array of file descriptors. When a file descriptor is
 * created, its pointer drops here. When a file descriptor is
 * closed, its place in this array is set to NULL and can be
 * taken by next ufs_open() call.
 */
//TODO: IMplement this definition
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
		file_descriptors = realloc(file_descriptors, file_descriptor_capacity);
	}
	file_descriptors[file_descriptor_count] = malloc(sizeof(struct filedesc));
	file_descriptors[file_descriptor_count]->file = file;
	file_descriptors[file_descriptor_count]->file_desc_value = file_descriptor_count + 1;
	return ++file_descriptor_count;
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
	size_t filename_size = strlen(new_file_name);
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
	/* IMPLEMENT THIS FUNCTION */
	(void)fd;
	(void)buf;
	(void)size;
	ufs_error_code = UFS_ERR_NOT_IMPLEMENTED;
	return -1;
}

ssize_t
ufs_read(int fd, char *buf, size_t size)
{
	/* IMPLEMENT THIS FUNCTION */
	(void)fd;
	(void)buf;
	(void)size;
	ufs_error_code = UFS_ERR_NOT_IMPLEMENTED;
	return -1;
}

int
ufs_close(int fd)
{
	if (fd <= file_descriptor_count && fd > 0) {
		file_descriptors[fd - 1]->file->refs--;
		// if (file_descriptors[fd - 1]->file->refs == 0) {
		// 	// TODO: Free the memory occupied by the file
		// 	free(file_descriptors[fd - 1]->file->block_list);
		// 	free(file_descriptors[fd - 1]->file->name);
		// 	struct file* prev = file_descriptors[fd - 1]->file->prev;
		// 	struct file* next = file_descriptors[fd - 1]->file->next;
		// 	if (prev != NULL && next != NULL) {
		// 		prev->next = next;
		// 		next->prev = prev;
		// 	} else if (prev != NULL) {
		// 		prev->next = NULL;
		// 	} else if (next != NULL) {
		// 		next->prev = NULL;
		// 	}
		// 	if (file_list == file_descriptors[fd - 1]->file) {
		// 		file_list = NULL;
		// 	}
		// 	free(file_descriptors[fd - 1]->file);
		// } 
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
	/* IMPLEMENT THIS FUNCTION */
	(void)filename;
	ufs_error_code = UFS_ERR_NOT_IMPLEMENTED;
	return -1;
}

void
ufs_destroy(void)
{
}
