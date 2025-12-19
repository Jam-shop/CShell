#ifndef KEYBOARDSHORTCUTS_H
#define KEYBOARDSHORTCUTS_H

#include "shellState.h"

void install_signal_handlers(ShellState* shell_state);
void handle_sigint(int sig);
void handle_sigtstp(int sig);
void cleanup_and_exit(ShellState* shell_state, int exit_status);

#endif