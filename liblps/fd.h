#pragma once

#include "linux.h"
#include "kfile.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/stat.h>

// Maximum number of file descriptors the LFI runtime process can have open.
#define LINUX_NOFILE 1024

struct FDTable {
    // File descriptor conversion table.
    int fds[LINUX_NOFILE];
    // File abstact layer
    struct FDFile *files[LINUX_NOFILE];
    // Full sandbox path for opened directories. This is necessary for
    // supporting fchdir(fd).
    char *dirs[LINUX_NOFILE];
    pthread_mutex_t lk;

    bool passthrough;
};

// Assign the host file descriptor to fd.
bool
fdassign(struct FDTable *t, int fd, int host_fd, char *dir);

// Returns the host file descriptor associated with fd.
int
fdget(struct FDTable *t, int fd);

char *
fddir(struct FDTable *t, int fd);

struct FDFile *
fdgetfile(struct FDTable *t, int fd);

// Adjust newfd so that it now points to oldfd. If newfd is -1, allocates a new
// file descriptor for newfd automatically (same behavior as dup).
int
fddup2(struct FDTable *t, int oldfd, int newfd);

// Close the host file descriptor associated with fd and remove the
// slot for fd in the table.
bool
fdclose(struct FDTable *t, int fd);

// Close the FDFile structure associated with fd and remove the
// slot for fd in the table.
bool
fdfclose(struct FDTable *t, int fd);

// Initialize the file descriptor table.
void
fdinit(struct LPSLinuxEngine *engine, struct FDTable *t);

// Closes all FDs in the table and frees associated memory.
void
fdfree(struct FDTable *t);

// find and assign FDFile in FDTable, return fd
int
fdfassign(struct FDTable *t, struct FDFile *f);
