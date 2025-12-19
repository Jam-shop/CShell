#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>
#include "../include/hop.h"


Hopstate init_hop_state () {
    Hopstate* state = malloc(sizeof(Hopstate));
    state->prev_dir = NULL;

    return *state;
}

int change_directory (Hopstate* state, const char* path) {
    // printf("I am in lol\n");
    char* current = getcwd(NULL, 0);
    if (current == NULL) {
        perror("getcwd");
        return -1;
    }

    // printf("\nCurrent path: %s\nRequested path: %s\n\n", current, path);

    if (chdir(path) != 0) {
        printf("No such directory!\n");
        free(current);
        return -1;
    }

    if (state->prev_dir) free(state->prev_dir);
    state->prev_dir = current;

    return 0;
}

int execute_hop_command (Hopstate* state, char** arguments, char** home_directory){
    if (arguments[1] == NULL){
        change_directory(state, *home_directory);
        return 0;
    }
    
    for (int i = 1; arguments[i] != NULL; i++) {
        // printf("Processing arguments[%d]: %s\n", i, arguments[i]);
        if (strcmp(arguments[i], "~") == 0){
            change_directory(state, *home_directory);
        }
        
        else if (strcmp(arguments[i], ".") == 0){
            continue;
        }
        
        else if (strcmp(arguments[i], "..") == 0) {
            char* current = getcwd(NULL, 0);
            if (current) {
                char* parent = dirname(current);
                change_directory(state, parent);
                free(current);
            }
        }
        
        else if (strcmp(arguments[i], "-") == 0){
            if (state->prev_dir){
                change_directory(state, state->prev_dir);
            }
        }
        
        else{
            // printf("idek why this is not working\n");
            change_directory(state, arguments[i]);
        }
    }

    return 0;
}

void cleanup_hop_state (Hopstate* state) {
    if (state->prev_dir) {
        free(state->prev_dir);
        state->prev_dir = NULL;
    }
}