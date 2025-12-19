#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/wait.h>

#include "../include/shellState.h"
#include "../include/shellInput.h"
#include "../include/sequential.h"
#include "../include/redirection.h"
#include "../include/bgExecution.h"

int execute_sequential_command (char* command, ShellState* shell_state, char* home_dir, int* should_exit) {
    char* start = command;
    char* end = command + strlen(command)-1;

    while (*start && isspace(*start)) start++;
    while (end > start && isspace(*end)) {
        *end = '\0';
        end--;
    }

    if (strlen(start) == 0) return 0;

    int bg = 0;
    if (strstr(start, "&") != NULL) bg = 1;

    char* tokens[256] = {NULL};
    int token_count = 0;
    char* cmd_copy = strdup(start);
    char* token = strtok(cmd_copy, " \t\n");

    while (token && token_count < 255) {
        tokens[token_count++] = token;
        token = strtok(NULL, " \t\n");
    }

    if (token_count == 0) {
        free(cmd_copy);
        return 0;
    }

    int builtin_result = execute_builtin_cmd(tokens, token_count, &shell_state->hop_state, home_dir, shell_state, should_exit);
    if (builtin_result != -1) {
        free(cmd_copy);
        return (builtin_result == 0) ? 0 : -1;
    }

    if (strstr(start, "|") != NULL) {
        int result = parse_redirection_cmd(start, &shell_state->hop_state, home_dir, shell_state, should_exit);
        free(cmd_copy);
        return result;
    }

    char** command_arr = malloc((token_count+1) * sizeof(char*));
    for (int i = 0; i < token_count; i++) command_arr[i] = tokens[i];
    command_arr[token_count] = NULL;

    int result;
    if (bg) {
        remove_bg_operator(command_arr);
        result = execute_background_commands(shell_state->bg_job_manager, shell_state->process_manager, command_arr);
    }
    else result = execute_foreground_commands(command_arr, shell_state->process_manager, shell_state->fg_job_manager);

    free(command_arr);
    free(cmd_copy);
    return result;
}

int parse_sequential_cmd (char* command_line, ShellState* shell_state, char* home_dir, int* should_exit) {
    char* cmd_str = strdup(command_line);
    if (!cmd_str) {
        perror ("strdup failed");
        return -1;
    }

    int exit_status = 0;
    char* saveptr;
    char* cmd_token = strtok_r(cmd_str, ";", &saveptr);

    while (cmd_token != NULL) {
        int result = execute_sequential_command(cmd_token, shell_state, home_dir, should_exit);
        if (result != 0) exit_status = result;
        cmd_token = strtok_r(NULL, ";", &saveptr);
    }

    free(cmd_str);
    return exit_status;
}