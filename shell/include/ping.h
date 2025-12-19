#ifndef PING_H
#define PING_H

int parse_ping_cmd (char** tokens, int token_count);
int execute_ping_cmd (long long int pid, long long int signal_number);

#endif