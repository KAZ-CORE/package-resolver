#ifndef CACHE_H
#define CACHE_H

#include "argument.h"
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

#include <solv/pool.h>
#include <solv/repo.h>
#include <solv/repo_deb.h>
#include <solv/repo_write.h>
#include <solv/repo_solv.h>
#include <solv/solver.h>


#define SIZE_ARRAY_ARCHS 9
extern const char *ARCHS[];

struct _delimiter_positions
{
    int pos1;
    int pos2;
};

struct _found_packages {
    const char **found;
    size_t size_found;

    const char **notfound;
    size_t size_notfound;

    bool error;
};

struct _found_depends {
    const char ***found;
    size_t *size_found;
    size_t size_size_found;

    const char **notfound;
    size_t size_notfound;

    bool error;
};

struct _filename {
    const char **filenames;
    size_t size_filenames;

    const char **locations;
    size_t size_locations;

    const char **notfound_pkgs;
    size_t size_notfound_pkgs;

    bool error;
};

typedef struct
{
    const char **found;
    size_t size_found;

    bool error;
} unique_depends;


typedef struct _delimiter_positions delimiter_positions;
typedef struct _found_packages found_packages;
typedef struct _filename filename;
typedef struct _found_depends found_depends;

static inline bool check_arch(const char* arch);
static inline bool check_file(const char *path);
static inline delimiter_positions get_delimiter_positions (
    const char* str,
    const char delimiter
);
static inline void get_arch(
    char *buffer,
    int size_buffer,
    delimiter_positions pos_dots,
    const char *str
);
static inline bool load_debpackages (
    Pool **pool,
    Repo **repo,
    const char* name_file,
    const char* name_repo,
    const char* arch
);
static inline bool load_solvpackages (
    Pool **pool,
    Repo **repo,
    const char* name_file
);
static inline found_packages find_packages(
    Packages packages,
    Pool **pool
);

bool create_cache (const char *name, const char *arch, const char* name_file);
bool show_find (Packages packages, const char* name_file);

bool show_filename (Packages packages, const char* name_file);

bool show_location (Packages packages, const char* name_file);

bool show_main_depends (Packages packages, const char* name_file);

bool show_main_depends__show_filename (
    Packages packages,
    const char* name_file
);
bool show_main_depends__show_location (
    Packages packages,
    const char* name_file
);

bool show_all_depends (Packages packages, const char* name_file);

bool show_all_depends__show_filename (
    Packages packages,
    const char* name_file
);
bool show_all_depends__show_location (
    Packages packages,
    const char* name_file
);

#endif
