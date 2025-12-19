#ifndef SHELLSTATE_H
#define SHELLSTATE_H

#include <sys/types.h>
#include "hop.h"

typedef struct BGJob {
    pid_t pid;
    char* command;
    int job_number;
    int completed;
    int stopped;
    struct BGJob* next_job;
} BGJob;

typedef struct {
    BGJob* jobs;
    int next_job_number;
    int job_count;
} BGJobManager;


typedef struct {
    pid_t fg_pgid;
    int fg_exists;
} FGJobManager;


typedef enum {
    PROCESS_RUNNING,
    PROCESS_STOPPED
} ProcessState;

typedef struct Process {
    pid_t pid;
    char* command;
    ProcessState state;
    struct Process* next_process;
} Process;

typedef struct {
    Process* processes;
    int process_count;
} ProcessManager;


typedef struct {
    Hopstate hop_state;
    BGJobManager* bg_job_manager;
    FGJobManager* fg_job_manager;
    ProcessManager* process_manager;
    int should_exit;
} ShellState;

BGJobManager* bg_job_manager_create();
void bg_job_manager_add (BGJobManager* jm, pid_t pid, const char* command);
void bg_job_manager_check_completed(BGJobManager* jm, ProcessManager* pm);
void bg_job_manager_cleanup (BGJobManager* jm);
void bg_job_manager_remove_completed (BGJobManager* jm);

ProcessManager* process_manager_create();
void process_manager_add (ProcessManager* pm, pid_t pid, const char* command);
void process_manager_update_status (ProcessManager* pm);
void process_manager_cleanup (ProcessManager* pm);
void process_manager_remove_process (ProcessManager* pm, pid_t pid);

FGJobManager* fg_job_manager_init ();
void fg_job_manager_set_job (FGJobManager* fm, pid_t pgid);
void fg_job_manager_clear_job (FGJobManager* fm);

#endif