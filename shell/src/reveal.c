#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <libgen.h>
#include <dirent.h>
#include "../include/reveal.h"
#include "../include/hop.h"


int compare_strings (const void* a, const void* b) {
    const char* s1 = *(const char**) a;
    const char* s2 = *(const char**) b;
    return strcmp(s1, s2);
}

void sort_strings(char** arr, int n) {
    qsort(arr, n, sizeof(char*), compare_strings);
}



long long int retrieve_dir_deets (char* path, char*** entries) {
    DIR* dir = opendir(path);

    if (dir == NULL) {
        // printf("Couldn't open directory!\n");
        return -1;
    }

    long long int number_of_entries = 0;
    struct dirent* dir_entry;
    while ((dir_entry = readdir(dir)) != NULL) number_of_entries++;

    char** names = malloc(number_of_entries * sizeof(char*));
    if (!names) {
        closedir(dir);
        return -1;
    }

    rewinddir(dir);
    long long int i = 0;
    while ((dir_entry = readdir(dir)) != NULL) names[i++] = strdup(dir_entry->d_name);

    closedir(dir);
    
    sort_strings(names, number_of_entries);
    *entries = names;

    return number_of_entries;
}

int execute_reveal_command (Hopstate* current_state, char** flags, char* path, char** home_directory) { 
    //Find out target directory
    char* target_dir = NULL;
    if (strcmp(path, "~") == 0){
        target_dir = strdup(*home_directory);
    }
    
    else if (path == NULL || strcmp(path, ".") == 0){
        target_dir = getcwd(NULL, 0);
    }
    
    else if (strcmp(path, "..") == 0) {
        char* current = getcwd(NULL, 0);
        if (current) {
            char* parent = dirname(current);
            target_dir = strdup(parent);
            free(current);
        }
        else target_dir = NULL;
    }
    
    else if (strcmp(path, "-") == 0){
        if (current_state->prev_dir){
            target_dir = strdup(current_state->prev_dir);
        }
        else {
            printf("No such directory!\n");
            return -1;
        }
    }
    
    else{
        target_dir = strdup(path);
    }

    //Retrieve directory contents
    char** entries;
    long long int entry_count = retrieve_dir_deets(target_dir, &entries);

    if (entry_count == -1) {
        printf("No such directory!\n");
        free(target_dir);
        return -1;
    }

    //Identifying flags
    bool a_flag = false;
    bool l_flag = false;
    bool valid_flags = true;

    for (int i = 0; flags[i] != NULL; i++) {
        if (flags[i][0] != '-') {
            valid_flags = false;
            break;
        }

        for (int j = 1; flags[i][j] != '\0'; j++) {
            if (flags[i][j] == 'a') a_flag = true;
            else if (flags[i][j] == 'l') l_flag = true;
            else {
                valid_flags = false;
                break;
            }
        }
        if (!valid_flags) break;
    }

    if (!valid_flags) {
        printf("Error: Invalid flag. Only -a and -l are supported.\n");
        return -1;
    }

    //Printing files and subdirectories
    sort_strings(entries, entry_count);

    for (long long int i = 0; i < entry_count; i++){
        if (!a_flag && strncmp(entries[i], ".", 1) == 0) continue;

        if (l_flag) printf("%s\n", entries[i]);
        else printf("%s    ", entries[i]);
    }
    if (!l_flag) printf("\n");

    free(target_dir);
    for (long long int i = 0; i < entry_count; i++) free(entries[i]);
    free(entries);

    return 0;
}

int parse_reveal_command (char* command_line, Hopstate* current_state, char** home_dir) {
    char* path = NULL;
    char* flags[256] = {NULL};
    int flag_count = 0;

    char* cmd_str = strdup(command_line);
    if (!cmd_str) {
        perror("strdup failed");
        return -1;
    }

    char* saveptr;
    char* token = strtok_r(cmd_str, " \t\n", &saveptr);

    if (token && strcmp(token, "reveal") == 0) token = strtok_r(NULL, " \t\n", &saveptr);

    while (token && flag_count < 255) {
        if (token[0] == '-') flags[flag_count++] = strdup(token);
        else {
            if (path != NULL) {
                printf("Error: Too many arguments.\n");
                free(path);
                for (int i = 0; i < flag_count; i++) free(flags[i]);
                free(cmd_str);
                return -1;
            }
            path = strdup(token);
        }
        token = strtok_r(NULL, " \t\n", &saveptr);
    }

    // printf("DEBUG: Path = '%s'\n", path ? path : "(null)");
    // for (int i = 0; i < flag_count; i++) printf("DEBUG: Flag[%d] = '%s'\n", i, flags[i]);
    int result = execute_reveal_command(current_state, flags, path, home_dir);

    free(path);
    for (int i = 0; i < flag_count; i++) free(flags[i]);
    free(cmd_str);

    return result;
}