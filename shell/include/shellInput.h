#ifndef SHELLINPUT_H
#define SHELLINPUT_H

#include "shellState.h"
#include "cfgParser.h"
#include "hop.h"
#include "reveal.h"
#include "log.h"
#include "redirection.h"
#include "sequential.h"

int process_command(char* command_line, ShellState* shell_state, char* home_dir, int* should_exit);
void initialise_shell(char** username, char** systemname, char** home_dir);
char* current_directory(char* home_dir);
void shell(char* username, char* systemname, char* home_dir);
void shell_cleanup(ShellState* state);
void cleanup(char* username, char* systemname, char* home_dir);
int execute_builtin_cmd (char** tokens, int token_count, Hopstate* hop_state, char* home_dir, ShellState* shell_state, int* should_exit);

#endif