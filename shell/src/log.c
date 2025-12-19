#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>
#include "../include/log.h"
#include "../include/shellInput.h"

static char log_file_path[1024] = {0};

void init_log_path() {
    if (log_file_path[0] == '\0') {
        char exe_path[1024];
        ssize_t len = readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1);
        
        if (len != -1) {
            exe_path[len] = '\0';
            char* exe_dir = dirname(exe_path);
            snprintf(log_file_path, sizeof(log_file_path), "%s/logs.txt", exe_dir);
            
            char resolved_path[1024];
            if (realpath(log_file_path, resolved_path) != NULL) {
                strncpy(log_file_path, resolved_path, sizeof(log_file_path) - 1);
            }
        }
    }
}

FILE* open_log_file(const char* mode) {
    init_log_path();
    return fopen(log_file_path, mode);
}

void write_to_log (char* command) {
    if (command == NULL || strlen(command) == 0) return;

    FILE* fptr_read = open_log_file("r");

    int number_of_lines = 0;
    char old_logs[15][256];
    
    if (fptr_read != NULL) {
        char buffer[256];
        while(fgets(buffer, sizeof(buffer), fptr_read) != NULL && number_of_lines < 15) {
            buffer[strcspn(buffer, "\n")] = '\0';

            if (strcmp(command, buffer) == 0) {
                continue;
            }

            strncpy(old_logs[number_of_lines], buffer, sizeof(old_logs[0]) - 1);
            old_logs[number_of_lines][sizeof(old_logs[0]) - 1] = '\0';
            number_of_lines++;
        }
        fclose(fptr_read);
    }

    // for (int i = 0; i<number_of_lines; i++) {
    //     printf("%s\n", old_logs[i]);
    // }

    FILE* fptr_write = open_log_file("w");
    if (fptr_write == NULL) {
        perror("Error opening log file\n");
        return;
    }

    int start_index = 0;
    if (number_of_lines == 15) start_index = 1;

    for (int i = start_index; i<number_of_lines; i++) fprintf(fptr_write, "%s\n", old_logs[i]);
    fprintf(fptr_write, "%s\n", command);

    fclose(fptr_write);
}

void delete_from_log () {
    FILE* fptr = open_log_file("w");
    if (fptr == NULL) perror("Error opening file.");
    fclose(fptr);
}

char* execute_command_from_log (int index) {
    FILE* fptr = open_log_file("r");
    if (fptr == NULL) {
        printf("Failed to open log file.\n");
        return NULL;
    }

    int number_of_lines = 0;
    int c;
    while((c = fgetc(fptr)) != EOF) if (c == '\n') number_of_lines++;
    rewind(fptr);

    if (index <= 0 || index > number_of_lines) {
        fclose(fptr);
        perror("Index provided is greater than the number of logs available.");
        return NULL;
    }

    int log_to_be_printed_index = number_of_lines - index;

    char buffer[256];
    char* result = NULL;
    int i = 0;

    while(fgets(buffer, sizeof(buffer), fptr) != NULL) {
        if (i == log_to_be_printed_index) {
            buffer[strcspn(buffer, "\n")] = '\0';
            result = malloc(strlen(buffer) + 1);

            if (result == NULL) {
                fclose(fptr);
                perror("Memory allocation failed.");
                return NULL;
            }

            strcpy(result, buffer);
            break;
        }
        i++;
    }

    fclose(fptr);
    if (result == NULL) perror("Failed to read the specified log entry.");

    return result;
} 

void print_log () {
    FILE* fptr = open_log_file("r");

    char buffer[256];
    while(fgets(buffer, 256, fptr) != NULL) printf("%s", buffer);

    fclose(fptr);
}

int parse_log_command (char* command_line, ShellState* shell_state, char** home_dir, int* should_exit) {
    char* cmd_str = strdup(command_line);
    if (!cmd_str) {
        perror("strdup failed");
        return -1;
    }

    char* saveptr;
    char* token = strtok_r(cmd_str, " \t\n", &saveptr);
    int valid = 1;

    if (token && strcmp(token, "log") == 0) token = strtok_r(NULL, " \t\n", &saveptr);

    if (token == NULL) print_log();
    else if (strcmp(token, "purge") == 0) {
        char* extra = strtok_r(NULL, " \t\n", &saveptr);
        if (extra != NULL) {
            fprintf(stderr, "Invalid Syntax! Purge doesn't take additional arguments.\n");
            valid = 0;
        }
        else delete_from_log();
    }
    else if (strcmp(token, "execute") == 0) {
        char* index_str = strtok_r(NULL, " \t\n", &saveptr);
        if (index_str == NULL) {
            fprintf(stderr, "Invalid Syntax! Expected an integer index.\n");
            valid = 0;
        }
        else {
            char* extra = strtok_r(NULL, " \t\n", &saveptr);
            if (extra) {
                fprintf(stderr, "Too many arguments!\n");
                valid = 0;
            }
            else {
                char* endptr;
                long index = strtol(index_str, &endptr, 10);

                if (*endptr != '\0') {
                    fprintf(stderr, "Error: '%s' is not a valid integer\n", index_str);
                    valid = 0;
                }
                else if (index < 0) {
                    fprintf(stderr, "Error: Index must be non-negative\n");
                    valid = 0;
                }
                else {
                    char* saved_command = execute_command_from_log((int)index);
                    if (saved_command) {
                        int result = process_command(saved_command, shell_state, *home_dir, should_exit);
                        if (result != 0) valid = 0;
                    }
                    else valid = 0;
                    free(saved_command);
                }
            }
        }
    }

    else {
        fprintf(stderr, "Invalid Syntax! Unknown subcommand '%s'\n", token);
        valid = 0;
    }

    free(cmd_str);
    return valid ? 0 : -1;
}