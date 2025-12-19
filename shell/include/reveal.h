#ifndef REVEAL_H
#define REVEAL_H
#include "hop.h"

int execute_reveal_command(Hopstate* current_state, char** flags, char* path, char** home_directory);
int parse_reveal_command (char* command_line, Hopstate* current_state, char** home_dir);

#endif