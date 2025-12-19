#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <errno.h>

#include "../include/ping.h"

int execute_ping_cmd (long long int pid, long long int signal_number) {
    if (kill((pid_t)pid, signal_number) == -1) {
        if (errno == ESRCH) printf("Np such process found\n");
        else perror("Error sending signal");
        return -1;
    }

    printf("Sent signal %d to process with pid %d", signal_number, pid);
    return 0;
}

int parse_ping_cmd (char** tokens, int token_count) {
    if (token_count != 3) {
        printf("Incorrect syntax!");
        return -1;
    }

    char *endptr;
    long long int pid = strtol(tokens[1], &endptr, 10);
    if (*endptr != '\0' || pid <= 0) {
        printf("Error: Invalid PID '%s'\n", tokens[1]);
        return -1;
    }

    long long int signal_number = strtol(tokens[2], &endptr, 10);
    if (*endptr != '\0') {
        printf("Error: Invalid signal number '%s'\n", tokens[2]);
        return -1;
    }

    int actual_signal_number = signal_number % 32;
    if (actual_signal_number < 0) {
        actual_signal_number += 32;
    }

    return execute_ping_cmd(pid, actual_signal_number);
}