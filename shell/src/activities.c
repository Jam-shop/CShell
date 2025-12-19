#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

#include "../include/shellState.h"
#include "../include/activities.h"

ProcessManager* process_manager_create() {
    ProcessManager* pm = malloc(sizeof(ProcessManager));
    if (!pm) return NULL;

    pm->processes = NULL;
    pm->process_count = 0;
    return pm;
}

void process_manager_add (ProcessManager* pm, pid_t pid, const char* command) {
    Process* new_process = malloc(sizeof(Process));
    if (!new_process) return;

    new_process->pid = pid;
    new_process->command = strdup(command);
    new_process->state = PROCESS_RUNNING;
    new_process->next_process = pm->processes;

    pm->processes = new_process;
    pm->process_count++;
}

void process_manager_update_status (ProcessManager* pm) {
    if (!pm) return;

    int status;
    Process* current = pm->processes;
    Process* prev = NULL;

    while (current) {
        pid_t result = waitpid(current->pid, &status, WNOHANG);

        if (result > 0) {
            Process* to_remove = current;

            if (prev) prev->next_process = current->next_process;
            else pm->processes = current->next_process;

            current = current->next_process;

            free(to_remove->command);
            free(to_remove);
            pm->process_count--;
            continue;
        }

        else if (result == 0) current->state = PROCESS_RUNNING;

        else {
            if (errno == ECHILD) {
                Process* to_remove = current;

                if (prev) prev->next_process = current->next_process;
                else pm->processes = current->next_process;

                current = current->next_process;

                free(to_remove->command);
                free(to_remove);
                pm->process_count--;
                continue;
            }
        }
        prev = current;
        if (current) current = current->next_process;
    }
}

void process_manager_remove_process (ProcessManager* pm, pid_t pid) {
    if (!pm) return;

    Process* current = pm->processes;
    Process* prev = NULL;

    while (current) {
        if (current->pid == pid) {
            Process* to_remove = current;
            if (prev) prev->next_process = current->next_process;
            else pm->processes = current->next_process;

            free(to_remove->command);
            free(to_remove);
            pm->process_count--;
            return;
        }

        prev = current;
        current = current->next_process;
    }
}


int compare_processes (const void* a, const void* b) {
    const Process* p1 = *(const Process**)a;
    const Process* p2 = *(const Process**)b;
    return strcmp(p1->command, p2->command);
}

void process_manager_list_activities (ProcessManager* pm) {
    process_manager_update_status(pm);

    if (pm->process_count == 0) {
        printf("No active processes.\n");
        return;
    }

    Process** process_array = malloc(pm->process_count * sizeof(Process*));
    if (!process_array) return;

    Process* current = pm->processes;
    int i = 0;
    while (current) {
        process_array[i++] = current;
        current = current->next_process;
    }

    qsort(process_array, pm->process_count, sizeof(Process*), compare_processes);

    for (i = 0; i < pm->process_count; i++) {
        const char* state_str = (process_array[i]->state == PROCESS_RUNNING) ? "Running" : "Stopped";
        printf("[%d] : %s - %s\n", process_array[i]->pid, process_array[i]->command, state_str);
    }

    free(process_array);
}

void process_manager_cleanup (ProcessManager* pm) {
    Process* current = pm->processes;
    Process* next;

    while (current) {
        next = current->next_process;
        free(current->command);
        free(current);
        current = next;
    }
    free(pm);
}