#include"spoor/spoor.h"

#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<sys/stat.h>

int main(int argc, char **argv)
{
    /* change current directory to database directory */
#ifdef _WIN32
    char *home_directory = getenv("USERPROFILE");
#else
    char *home_directory = getenv("HOME");
#endif

    char *database_path = malloc((strlen(home_directory) + 7 + 1) * sizeof(*database_path));
    printf("%s\n", home_directory);
#ifdef RELEASE
    strcpy(database_path, home_directory);
    strcpy(database_path + strlen(home_directory), "/.spoor");
#else 
    strcpy(database_path, ".spoor");
#endif
#ifdef _WIN32
    mkdir(database_path);
#else
    mkdir(database_path, 0777);
#endif
    chdir(database_path);

    free(database_path);

    if (argc <= 1)
        spoor_ui_object_show();
    else if (strcmp(argv[1], "--gui") == 0)
        spoor_ui_raylib_object_show();
    else if (strcmp(argv[1], "-v") == 0)
    {
        printf("SPOOR VERSION: %d.%d.%d\n",
               SPOOR_VERSION_MAJOR, 
               SPOOR_VERSION_MINOR,
               SPOOR_VERSION_PATCH);
    }
    else if (strcmp(argv[1], "-c") == 0)
    {
        char *arguments = spoor_object_argv_to_command(argc - 2,
                                                       argv + 2);
        printf("Arguments %s\n", arguments);
        SpoorObject *spoor_object = spoor_object_create(arguments);
        spoor_storage_save(spoor_object);
        spoor_debug_spoor_object_print(spoor_object);
        free(spoor_object);
    }

    return 0;
}
