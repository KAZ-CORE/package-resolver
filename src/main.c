#include <stdbool.h>
#include "headers/argument.h"

int main(int argc, char *argv[])
{

    tribool result = TRIBOOL_FALSE;

    result = handle_create_cache(argc,argv);
    if (result == TRIBOOL_TRUE)
        return 0;

    if (result == TRIBOOL_UNKNOWN)
        return 1;


    result = handle_use_cache(argc, argv);
    if (result == TRIBOOL_TRUE)
        return 0;

    if (result == TRIBOOL_UNKNOWN)
        return 1;

    fprintf(stderr,
        "Invalid input\n");
    return 1;
}
