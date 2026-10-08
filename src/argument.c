#include "headers/argument.h"
#include "headers/cache.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

static inline bool has_pipeline(void)
{
    struct stat st;

    return fstat(STDIN_FILENO, &st) == 0 &&
           S_ISFIFO(st.st_mode);
}

static inline bool read_pipeline(
    struct pipeline *pipeline,
    size_t size
)
{
    if (size == 0)
        return false;

    size_t capacity = PIPELINE_BUFFER_SIZE;

    pipeline->items = malloc(
        size * sizeof(char *) + capacity
    );

    if (!pipeline->items)
        return false;

    char *data = (char *)(pipeline->items + size);

    size_t data_size = 0;
    size_t count = 0;

    char *line = NULL;
    size_t line_capacity = 0;
    ssize_t line_length;

    while (
        count < size &&
        (line_length = getline(
            &line,
            &line_capacity,
            stdin
        )) != -1
    )
    {
        if (line_length > 0 && line[line_length - 1] == '\n')
            line[--line_length] = '\0';

        char *token = strtok(line, " ");

        while (token && count < size)
        {
            size_t token_length = strlen(token) + 1;

            if (data_size + token_length > capacity)
            {
                size_t new_capacity = capacity;

                while (data_size + token_length > new_capacity)
                    new_capacity *= 2;

                size_t items_size = size * sizeof(char *);

                char *new_items = realloc(
                    pipeline->items,
                    items_size + new_capacity
                );

                if (!new_items)
                {
                    free(line);
                    free(pipeline->items);
                    pipeline->items = NULL;
                    return false;
                }

                pipeline->items = (char **)new_items;
                data = (char *)(pipeline->items + size);
                capacity = new_capacity;
            }

            memcpy(
                data + data_size,
                token,
                token_length
            );

            data_size += token_length;
            count++;

            token = strtok(NULL, " ");
        }
    }

    free(line);

    pipeline->count = count;

    if (count == 0)
    {
        free(pipeline->items);
        pipeline->items = NULL;
        return false;
    }

    char *ptr = data;

    for (size_t i = 0; i < count; i++)
    {
        pipeline->items[i] = ptr;
        ptr += strlen(ptr) + 1;
    }

    return true;
}


static inline bool is_flag_at_position(
    const char* main_flag,
    int pos,
    int argc,
    char *argv[]
)
{
    return argc > pos && strcmp(argv[pos], main_flag) == 0;
}

static inline bool argc_in_range(int argc, int min, int max)
{
    return argc >= min && argc <= max;
}

static inline tribool check_flag (
    int argc,
    char *argv[],
    const char* name_flag,
    int pos,
    int min,
    int max
)
{
    if (!is_flag_at_position(name_flag,pos,argc,argv))
        return TRIBOOL_FALSE;

    if (!argc_in_range(argc,min,max))
        return TRIBOOL_UNKNOWN;

    return TRIBOOL_TRUE;
}

static inline tribool check_flags (
    int argc,
    char *argv[],
    char** const name_flags,
    int num_flags,
    int * const pos,
    int min,
    int max
)
{
    for (int i = 0; i < num_flags; i++) {
        if (!is_flag_at_position(name_flags[i],pos[i],argc,argv))
            return TRIBOOL_FALSE;
    }

    if (!argc_in_range(argc,min,max))
        return TRIBOOL_UNKNOWN;

    return TRIBOOL_TRUE;
}

static inline tribool handle_flag (
    int argc,
    char *argv[],
    bool (*func)(Packages, const char *),
    const char * name_flag,
    int pos,
    int min,
    int max
)
{

    tribool result = check_flag(argc, argv, name_flag, pos, min, max);

    if (result == TRIBOOL_FALSE)
        return result;

    if (result == TRIBOOL_UNKNOWN) {

        if (argc == (min-1) && has_pipeline()) {
            struct pipeline pipeline = {0};
            if (!read_pipeline(&pipeline, MAX_PIPELINE_ITEMS)) {
                fprintf(stderr,
                        "Pipeline error:\n"
                        "    Failed to read pipeline input\n"
                );
                free(pipeline.items);
                return TRIBOOL_FALSE;
            }
            if(!func((Packages){pipeline.items, pipeline.count}, argv[1]))
            {
                free(pipeline.items);
                return TRIBOOL_UNKNOWN;
            }
            free(pipeline.items);
            return TRIBOOL_TRUE;
        }

        return TRIBOOL_UNKNOWN;
    }

    if(!func((Packages){&argv[min-1], argc-min+1},argv[1]))
        return TRIBOOL_UNKNOWN;
    return TRIBOOL_TRUE;
}

static inline tribool handle_flags (
    int argc,
    char *argv[],
    bool (*func)(Packages, const char *),
    char **const name_flags,
    int num_flags,
    int *const pos,
    int min,
    int max
)
{

    tribool result = check_flags(
        argc,
        argv,
        name_flags,
        num_flags,
        pos,
        min,
        max
    );

    if (result == TRIBOOL_FALSE)
        return result;

    if (result == TRIBOOL_UNKNOWN) {
        if (argc == (min-1) && has_pipeline()) {
            struct pipeline pipeline = {0};
            if (!read_pipeline(&pipeline, MAX_PIPELINE_ITEMS)) {
                fprintf(stderr,
                        "Pipeline error:\n"
                        "    Failed to read pipeline input\n"
                );
                free(pipeline.items);
                return TRIBOOL_FALSE;
            }
            if(!func((Packages){pipeline.items, pipeline.count}, argv[1]))
            {
                free(pipeline.items);
                return TRIBOOL_UNKNOWN;
            }

            free(pipeline.items);
            return TRIBOOL_TRUE;
        }

        return TRIBOOL_UNKNOWN;
    }

    if(!func((Packages){&argv[min-1], argc-min+1},argv[1]))
        return TRIBOOL_UNKNOWN;
    return TRIBOOL_TRUE;
}

static inline void character_generation(
    char character,
    char *buffer,
    size_t size
)
{
    for (size_t i = 0; i < size; i++)
        buffer[i] = character;

    buffer[size] = '\0';
}

static inline tribool handle_all_flags (
    int argc,
    char **argv,
    tribool (*handlers[]) (int, char *[]),
    size_t size_handlers
)
{
    for (size_t i = 0; i < size_handlers; i++)
    {
        tribool flag_result = handlers[i](argc,argv);
        if ( flag_result == TRIBOOL_TRUE ) return TRIBOOL_TRUE;
        if ( flag_result == TRIBOOL_UNKNOWN ) return TRIBOOL_UNKNOWN;
    }

    fprintf(stderr,
        "Invalid cache operation: %s\n", argv[3]);
    return TRIBOOL_UNKNOWN;
}

static inline tribool define_flag_combinations(
    int argc,
    char **argv,
    char *main_flag,
    bool (*main_handler) (Packages, const char *),
    char **flags,
    size_t size_flags,
    bool (*handlers[])(Packages, const char *),
    size_t size_handlers
)
{

    if (size_flags != size_handlers)
        return TRIBOOL_UNKNOWN;

    tribool result = TRIBOOL_FALSE;
    int MIN = 6;

    for (size_t i = 0; i < size_handlers; i++)
    {
        result = handle_flags(
            argc,
            argv,
            handlers[i],
            (char *[]) {main_flag,flags[i]},
            2,            //NUM_FLAGS
            (int[]){3,4}, //POS
            MIN,          //MIN
            INT_MAX       //MAX
        );

        if (result == TRIBOOL_TRUE)
            return TRIBOOL_TRUE;

        if (result == TRIBOOL_UNKNOWN) {
            if (MIN-1 == argc) {
                size_t size_spaces = strlen(flags[i]) + 1;
                char spaces[size_spaces];
                character_generation(' ', spaces, size_spaces-1);

                fprintf(
                    stderr,
                    "Missing argument:\n"
                    "    ./main FILE %s %s PACKAGES\n"
                    "                    %s^^^^^^^^\n"
                    "                            PACKAGES are required\n",
                    main_flag,
                    flags[i],
                    spaces
                );
            }
            return TRIBOOL_UNKNOWN;
        }

    }


    // main flag
    MIN=5;
    result = handle_flag(
        argc,
        argv,
        main_handler,
        main_flag,
        3,      // POS
        MIN,    // MIN
        INT_MAX // MAX
    );

    if (result == TRIBOOL_TRUE)
        return TRIBOOL_TRUE;

    if (result == TRIBOOL_UNKNOWN)
    {
        if (MIN-1 == argc) {
            size_t size_spaces = strlen(main_flag) + 1;
            char spaces[size_spaces];
            character_generation(' ', spaces, size_spaces-1);
            fprintf(
                stderr,
                "Missing argument:\n"
                "    ./main FILE  %s PACKAGES\n"
                "                  %s^^^^^^^^\n"
                "                            PACKAGES are required\n",
                main_flag,
                spaces
            );
        }
        return TRIBOOL_UNKNOWN;
    }

    return TRIBOOL_FALSE;

}


static inline tribool handle_show_filename (int argc, char *argv[])
{
    int MIN = 5;
    tribool result = handle_flag(
        argc,
        argv,
        show_filename,
        "--show-filename",
        3,      // POS
        5,      // MIN
        INT_MAX // MAX
    );

    if (result == TRIBOOL_TRUE)
        return TRIBOOL_TRUE;

    if (result == TRIBOOL_UNKNOWN)
    {
        if (MIN-1 == argc) {
            fprintf(
                stderr,
                "Missing argument:\n"
                "    ./main FILE --use-cache --show-filename PACKAGES\n"
                "                                            ^^^^^^^^\n"
                "                            PACKAGES are required\n"
            );
        }
        return TRIBOOL_UNKNOWN;
    }

    return TRIBOOL_FALSE;

}

static inline tribool handle_show_location (int argc, char *argv[])
{
    int MIN = 5;
    tribool result = handle_flag(
        argc,
        argv,
        show_location,
        "--show-location",
        3,      // POS
        5,      // MIN
        INT_MAX // MAX
    );

    if (result == TRIBOOL_TRUE)
        return TRIBOOL_TRUE;

    if (result == TRIBOOL_UNKNOWN)
    {
        if (MIN-1 == argc) {
            fprintf(
                stderr,
                "Missing argument:\n"
                "    ./main FILE --use-cache --show-location PACKAGES\n"
                "                                            ^^^^^^^^\n"
                "                            PACKAGES are required\n"
            );
        }
        return TRIBOOL_UNKNOWN;
    }

    return TRIBOOL_FALSE;

}



static inline tribool handle_show_main_depends (int argc, char *argv[])
{

    return define_flag_combinations(
        argc,
        argv,
        "--show-main-depends",
        show_main_depends,
        (char *[]) {
            "--show-filename",
            "--show-location"
        },
        2,
        (bool (*[])(Packages, const char *)) {
            show_main_depends__show_filename,
            show_main_depends__show_location
        },
        2
    );
}

static inline tribool handle_show_all_depends (int argc, char *argv[])
{

    return define_flag_combinations(
        argc,
        argv,
        "--show-all-depends",
        show_all_depends,
        (char *[]) {
            "--show-filename",
            "--show-location"
        },
        2,
        (bool (*[])(Packages, const char *)) {
            show_all_depends__show_filename,
            show_all_depends__show_location
        },
        2
    );
}

static inline tribool handle_find (int argc, char *argv[])
{
    int MIN = 5;
    tribool result = handle_flag(
        argc,
        argv,
        show_find,
        "--find",
        3,      // POS
        5,      // MIN
        INT_MAX // MAX
    );

    if (result == TRIBOOL_TRUE)
        return TRIBOOL_TRUE;

    if (result == TRIBOOL_UNKNOWN)
    {
        if (MIN-1 == argc) {
            fprintf(
                stderr,
                "Missing argument:\n"
                "    ./main FILE --use-cache --find PACKAGES\n"
                "                                   ^^^^^^^^\n"
                "                            PACKAGES are required\n"
            );
        }
        return TRIBOOL_UNKNOWN;
    }

    return TRIBOOL_FALSE;

}


tribool handle_create_cache (int argc, char *argv[])
{

    int min = 5;
    tribool result = check_flag(argc, argv, "--create-cache", 2, min, INT_MAX);

    if (result == TRIBOOL_FALSE) {

        if (argc <= 1)
            return TRIBOOL_FALSE;

        if (strcmp(argv[1],"--create-cache") == 0) {
            fprintf(
                stderr,
                "Invalid argument order:\n"
                "    ./main FILE --create-cache NAME ARCH\n"
                "           ^^^^\n"
                "         FILE must appear before --create-cache\n"
            );
            return TRIBOOL_UNKNOWN;
        }

        return TRIBOOL_FALSE;
    }


    if (result == TRIBOOL_UNKNOWN) {
        if (argc == min-1) {
            fprintf(
                stderr,
                "Missing arguments:\n"
                "    ./main FILE --create-cache NAME ARCH\n"
                "                                    ^^^^\n"
                "                          ARCH is required\n"
            );
        } else {
            fprintf(
                stderr,
                "Missing arguments:\n"
                "    ./main FILE --create-cache NAME ARCH\n"
                "                               ^^^^ ^^^^\n"
                "                         NAME and ARCH are required\n"
            );
        }

        return TRIBOOL_UNKNOWN;
    }

    if (!create_cache(argv[3], argv[4], argv[1]))
        return TRIBOOL_UNKNOWN;

    return TRIBOOL_TRUE;
}

tribool handle_use_cache (int argc, char *argv[])
{
    int min = 4;
    tribool result = check_flag(argc, argv, "--use-cache", 2, min, INT_MAX);

    if (result == TRIBOOL_FALSE) {

        if (argc <= 1)
            return TRIBOOL_FALSE;

        if (strcmp(argv[1],"--use-cache") == 0) {
            fprintf(
                stderr,
                "Invalid argument order:\n"
                "    ./main FILE --use-cache OPERATION ...\n"
                "           ^^^^\n"
                "         FILE must appear before --use-cache\n"
            );
            return TRIBOOL_UNKNOWN;
        }

        return TRIBOOL_FALSE;
    }

    if (result == TRIBOOL_UNKNOWN) {
        fprintf(
            stderr,
            "Missing argument:\n"
            "    ./main FILE --use-cache OPERATION\n"
            "                            ^^^^^^^^^\n"
            "                       OPERATION is required\n"
        );
        return TRIBOOL_UNKNOWN;
    }

    tribool flag_result = TRIBOOL_FALSE;


    return handle_all_flags(
        argc,
        argv,
         (tribool (*[]) (int, char *[])) {
            handle_find,
            handle_show_filename,
            handle_show_location,
            handle_show_main_depends,
            handle_show_all_depends
        },
        5
    );
}
