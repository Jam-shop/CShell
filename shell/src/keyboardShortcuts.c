#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

#include "../include/shellState.h"
#include "../include/shellInput.h"
#include "../include/keyboardShortcuts.h"

static ShellState* shell_state_global = NULL;

void handle_sigint (int sig) {
    if (!shell_state_global) return;
    printf("\n");

    if (shell_state_global->fg_job_manager->fg_exists && shell_state_global->fg_job_manager->fg_pgid > 0) kill(-shell_state_global->fg_job_manager->fg_pgid, SIGINT);
    else {
        //COME BACK TO THIS LATER
        fflush(stdout);
    }
}

void handle_sigtstp (int sig) {
    if (!shell_state_global) return;

    if (shell_state_global->fg_job_manager->fg_exists && shell_state_global->fg_job_manager->fg_pgid > 0) {
        kill(-shell_state_global->fg_job_manager->fg_pgid, SIGTSTP);

        Process* current = shell_state_global->process_manager->processes;
        while (current) {
            if (current->pid == shell_state_global->fg_job_manager->fg_pgid) {
                current->state = PROCESS_STOPPED;
                printf("\n[%d] Stopped %s\n", current->pid, current->command);
                break;
            }
            current = current->next_process;
        }
        fg_job_manager_clear_job(shell_state_global->fg_job_manager);
    }
}

void cleanup_and_exit (ShellState* shell_state, int exit_status) {
    if (!shell_state) return;

    Process* current = shell_state->process_manager->processes;
    while (current) {
        kill(current->pid, SIGKILL);
        current = current->next_process;
    }
    shell_cleanup(shell_state);
    printf("\nlogout\n");
    exit(exit_status);
}

void install_signal_handlers (ShellState* shell_state) {
    shell_state_global = shell_state;

    // Ctrl+C
    struct sigaction sa_int;
    sa_int.sa_handler = handle_sigint;
    sigemptyset(&sa_int.sa_mask);
    sa_int.sa_flags = SA_RESTART;

    if (sigaction(SIGINT, &sa_int, NULL) == -1) {
        perror("sigaction SIGINT");
        exit(EXIT_FAILURE);
    }

    // Ctrl+Z
    struct sigaction sa_tstp;
    sa_tstp.sa_handler = handle_sigint;
    sigemptyset(&sa_tstp.sa_mask);
    sa_tstp.sa_flags = SA_RESTART;

    if (sigaction(SIGTSTP, &sa_tstp, NULL) == -1) {
        perror("sigaction SIGTSTP");
        exit(EXIT_FAILURE);
    }

    signal(SIGQUIT, SIG_IGN);
    signal(SIGTTOU, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
}


FGJobManager* fg_job_manager_init () {
    FGJobManager* fm = malloc(sizeof(FGJobManager));
    fm->fg_pgid = 0;
    fm->fg_exists = 0;

    return fm;
}

void fg_job_manager_set_job (FGJobManager* fm, pid_t pgid) {
    fm->fg_pgid = pgid;
    fm->fg_exists = 1;

    tcsetpgrp(STDIN_FILENO, pgid);
}

void fg_job_manager_clear_job (FGJobManager* fm) {
    tcsetpgrp(STDIN_FILENO, getpgrp());

    fm->fg_pgid = 0;
    fm->fg_exists = 0;
}