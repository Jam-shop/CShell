#ifndef LOG_H
#define LOG_H
#include "shellState.h"

int parse_log_command (char* command_line, ShellState* shell_state, char** home_dir, int* should_exit);
void write_to_log (char* command);
void delete_from_log ();
char* execute_command_from_log (int index);
void print_log ();

#endif