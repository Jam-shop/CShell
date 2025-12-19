#ifndef SEQUENTIAL_H
#define SEQUENTIAL_H

#include "shellState.h"

typedef struct {
    const char *command;
    int exit_code;
    char* output;
} CommandResult;

int execute_sequential_command (char* command, ShellState* shell_state, char* home_dir, int* should_exit);
int parse_sequential_cmd (char* command_line, ShellState* shell_state, char* home_dir, int* should_exit);

#endif