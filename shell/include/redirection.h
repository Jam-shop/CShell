#ifndef REDIRECTION_H
#define REDIRECTION_H

#include "shellState.h"
#include "redirection.h"
#include "hop.h"
#include "reveal.h"
#include "log.h"

int process_redirection (char** tokens, int token_count);
int execute_pipeline(char*** commands, int n, Hopstate* hop_state, char* home_dir, ShellState* shell_state, int* should_exit);
int execute_one_cmd(char** command, Hopstate* hop_state, char* home_dir, ShellState* shell_state, int* should_exit);
// int execute_builtin_cmd (char** tokens, int token_count, Hopstate* hop_state, char* home_dir, ShellState* shell_state, int* should_exit);
int parse_redirection_cmd (char* command_line, Hopstate* hop_state, char* home_dir, ShellState* shell_state, int* should_exit);
int handle_input_redirection (char* filename);
int handle_output_redirection (char* filename, int append_flag);

#endif