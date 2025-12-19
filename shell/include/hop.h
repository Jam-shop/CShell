#ifndef HOP_H
#define HOP_H

typedef struct {
    char* prev_dir;
} Hopstate;

Hopstate init_hop_state ();
int execute_hop_command (Hopstate* state, char** arguments, char** home_directory);
void cleanup_hop_state (Hopstate* state);

#endif