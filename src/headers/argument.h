#ifndef ARGUMENT_H
#define ARGUMENT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <limits.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdint.h>

#define MAX_SUPPORT_PACKAGS 40000
#define MAX_PIPELINE_ITEMS MAX_SUPPORT_PACKAGS
#define MAX_PACKAGE_NAME 256
#define PIPELINE_BUFFER_SIZE 4096

struct pipeline {
    char **items;
    size_t count;
};

struct _Packages {
    char **names;
    int count;
};

typedef struct _Packages Packages;

#define TRIBOOL_FALSE    0
#define TRIBOOL_UNKNOWN -1
#define TRIBOOL_TRUE     1

typedef int8_t tribool;

static inline bool has_pipeline(void);

static inline bool read_pipeline(
    struct pipeline *pipeline,
    size_t size
);
static inline bool is_flag_at_position(
    const char* main_flag,
    int pos,
    int argc,
    char *argv[]
);
static inline bool argc_in_range(int argc,
    int min,
    int max
);

static inline tribool check_flag (
    int argc,
    char *argv[],
    const char* name_flag,
    int pos,
    int min,
    int max
);
static inline tribool check_flags (
    int argc,
    char *argv[],
    char** const name_flags,
    int num_flags,
    int * const pos,
    int min,
    int max
);
static inline tribool handle_flag (
    int argc,
    char *argv[],
    bool (*func)(Packages, const char *),
    const char * name_flag,
    int pos,
    int min,
    int max
);
static inline tribool handle_flags (
    int argc,
    char *argv[],
    bool (*func)(Packages, const char *),
    char **const name_flags,
    int num_flags,
    int *const pos,
    int min,
    int max
);
static inline tribool handle_show_url (
    int argc,
    char *argv[]
);
static inline tribool handle_show_main_depends (
    int argc,
    char *argv[]
);
static inline tribool handle_show_all_depends (
    int argc,
    char *argv[]
);
static inline tribool handle_find (
    int argc,
    char *argv[]
);

tribool handle_create_cache (int argc, char *argv[]);
tribool handle_use_cache (int argc, char *argv[]);

#endif
