#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>

#include "../include/shellState.h"
#include "../include/bgExecution.h"

BGJobManager* bg_job_manager_create() {
    BGJobManager* jm = malloc(sizeof(BGJobManager));
    if (!jm) return NULL;

    jm->jobs = NULL;
    jm->next_job_number = 1;
    jm->job_count = 0;

    return jm;
}

BGJob* bg_job_manager_find_job(BGJobManager* jm, int job_number) {
    if (!jm) return NULL;

    BGJob* current = jm->jobs;
    while (current) {
        if (current->job_number == job_number && !current->completed) return current;
        current = current->next_job;
    }
    return NULL;
}

BGJob* bg_job_manager_get_latest_job(BGJobManager* jm) {
    if (!jm || !jm->jobs) return NULL;

    BGJob* latest = jm->jobs;
    BGJob* current = jm->jobs;

    while (current) {
        if (current->job_number > latest->job_number && !current->completed) latest = current;
        current = current->next_job;
    }

    return latest;
}

void bg_job_manager_add (BGJobManager* jm, pid_t pid, const char* command) {
    BGJob* new_job = malloc(sizeof(BGJob));
    if (!new_job) return;

    new_job->pid = pid;
    new_job->command = strdup(command);
    new_job->job_number = jm->next_job_number;
    new_job->completed = 0;
    new_job->stopped = 0;
    new_job->next_job = jm->jobs;

    jm->jobs = new_job;
    jm->next_job_number++;
    jm->job_count++;

    printf("[%d] %d\n", new_job->job_number, new_job->pid);
}

void bg_job_manager_check_completed(BGJobManager* jm, ProcessManager* pm) {
    if (!jm) return;

    BGJob* current = jm->jobs;
    int status;
    pid_t pid;

    while (current) {
        if (!current->completed) {
            pid = waitpid(current->pid, &status, WNOHANG | WUNTRACED | WCONTINUED);
            
            if (pid > 0) {
                if (WIFEXITED(status) || WIFSIGNALED(status)) {
                    current->completed = 1;
                    if (WIFEXITED(status)) printf("%s with pid %d exited normally\n", current->command, current->pid);
                    else if (WIFSIGNALED(status)) printf("%s with pid %d exited abnormally.\n", current->command, current->pid);

                    if (pm) process_manager_remove_process(pm, current->pid);
                }

                else if (WIFSTOPPED(status)) {
                    current->stopped = 1;
                    if (pm) {
                        Process* proc = pm->processes;
                        while (proc) {
                            if (proc->pid == current->pid) {
                                proc->state = PROCESS_STOPPED;
                                break;
                            }
                            proc = proc->next_process;
                        }
                    }
                    printf("\n[%d] Stopped %s\n", current->job_number, current->command);
                }

                else if (WIFCONTINUED(status)) {
                    current->stopped = 0;
                    if (pm) {
                        Process* proc = pm->processes;
                        while (proc) {
                            if (proc->pid == current->pid) {
                                proc->state = PROCESS_RUNNING;
                                break;
                            }
                            proc = proc->next_process;
                        }
                    }
                }
            }
        }
        current = current->next_job;
    }
    bg_job_manager_remove_completed(jm);
}

void bg_job_manager_cleanup (BGJobManager* jm) {
    if (!jm) return;

    BGJob* current = jm->jobs;
    BGJob* next;

    while (current) {
        next = current->next_job;
        free(current->command);
        free(current);
        current = next;
    }
    free(jm);
}

void bg_job_manager_remove_completed (BGJobManager* jm) {
    if (!jm) return;

    BGJob** current = &jm->jobs;

    while (*current) {
        if ((*current)->completed) {
            BGJob* to_remove = *current;
            *current = to_remove->next_job;
            free(to_remove->command);
            free(to_remove);
            jm->job_count--;
        }
        else current = &(*current)->next_job;
    }
}


int fg_cmd (ProcessManager* pm, BGJobManager* jm, FGJobManager* fm, char** args) {
    if (!pm || !jm || !fm) return -1;
    BGJob* job = NULL;

    if (args[1] == NULL) {
        job = bg_job_manager_get_latest_job(jm);
        if (!job) {
            printf("No such job\n");
            return -1;
        }
        else {
            char* endptr;
            long job_num = strtol(args[1], &endptr, 10);
            if (*endptr != '\0' || job_num <= 0) {
                printf("No such job\n");
                return -1;
            }
        }
    }

    printf("%s\n", job->command);

    if (job->stopped) {
        if (kill(job->pid, SIGCONT) == -1) {
            perror("kill SIGCONT");
            return -1;
        }
        job->stopped = 0;

        Process* proc = pm->processes;
        while (proc) {
            if (proc->pid == job->pid) {
                proc->state = PROCESS_RUNNING;
                break;
            }
            proc = proc->next_process;
        }
    }

    fg_job_manager_set_job(fm, job->pid);

    int status;
    pid_t result = waitpid(job->pid, &status, WUNTRACED);

    fg_job_manager_clear_job(fm);

    if (result > 0) {
        if (WIFSTOPPED(status)) {
            job->stopped = 1;

            Process* proc = pm->processes;
            while (proc) {
                if (proc->pid == job->pid) {
                    proc->state = PROCESS_STOPPED;
                    break;
                }
                proc = proc->next_process;
            }
            printf("\n[%d] Stopped %s\n", job->job_number, job->command);
            return 0;
        }

        else if (WIFEXITED(status)) {
            job->completed = 1;
            process_manager_remove_process(pm, job->pid);
            return WEXITSTATUS(status);
        }

        else if (WIFSIGNALED(status)) {
            job->completed = 1;
            process_manager_remove_process(pm, job->pid);
            return -1;
        }
    }

    return -1;
}

int bg_cmd (ProcessManager* pm, BGJobManager* jm, char** args) {
    if (!jm || !pm) return -1;
    BGJob* job = NULL;

    if (args[1] == NULL) {
        job = bg_job_manager_get_latest_job(jm);
        if (!job) {
            printf("No such job\n");
            return -1;
        }
    }
    else {
        char* endptr;
        long job_num = strtol(args[1], &endptr, 10);
        if (*endptr != '\0' || job_num <= 0) {
            printf("Invalid job number: %s\n", args[1]);
            return -1;
        }

        job = bg_job_manager_find_job(jm, (int)job_num);
        if (!job) {
            printf("No such job\n");
            return -1;
        }
    }

    if (!job->stopped) {
        printf("Job already running\n");
        return -1;
    }

    if (kill(job->pid, SIGCONT) == -1) {
        perror("kill SIGCONT");
        return -1;
    }

    job->stopped = 0;

    Process* proc = pm->processes;
    while (proc) {
        if (proc->pid == job->pid) {
            proc->state = PROCESS_RUNNING;
            break;
        }
        proc = proc->next_process;
    }

    printf("[%d] %s &\n", job->job_number, job->command);

    return 0;
}


int execute_background_commands (BGJobManager* jm, ProcessManager* pm, char** args) {
    pid_t pid = fork();
    
    if (pid == 0) {
        setpgid(0, 0);
        signal(SIGTTIN, SIG_IGN);
        signal(SIGTTOU, SIG_IGN);

        execvp(args[0], args);

        fprintf(stderr, "%s: command not found\n", args[0]);
        exit(127);
    }
    else if (pid > 0) {
        bg_job_manager_add(jm, pid, args[0]);
        process_manager_add(pm, pid, args[0]);
        return 0;
    }
    else {
        perror("fork");
        return -1;
    }

    // return -1;
}

int execute_foreground_commands (char** args, ProcessManager* pm, FGJobManager* fm) {
    pid_t pid = fork();

    if (pid == 0) {
        setpgid(0, 0);

        signal(SIGINT, SIG_DFL);
        signal(SIGTSTP, SIG_DFL);
        signal(SIGQUIT, SIG_DFL);

        execvp(args[0], args);
        fprintf(stderr, "%s: command not found\n", args[0]);
        exit(127);
    }
    else if (pid > 0) {
        fg_job_manager_set_job(fm, pid);

        int status;
        waitpid(pid, &status, WUNTRACED);

        fg_job_manager_clear_job(fm);

        if (WIFEXITED(status)) return WEXITSTATUS(status);
        if (WIFSIGNALED(status)) return -1;
        if (WIFSTOPPED(status)) return -1;

        return -1;
    }
    else {
        perror("fork");
        return -1;
    }

    // return -1;
}

int is_bg_cmd (char** args) {
    for (int i = 0; args[i] != NULL; i++) if (strcmp(args[i], "&") == 0) return 1;

    return 0;
}

void remove_bg_operator (char** args) {
    for (int i = 0; args[i] != NULL; i++) {
        if (strcmp(args[i], "&") == 0) {
            // free(args[i]);
            args[i] = NULL;
            return;
        }
    }
}                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          