#ifndef BGEXECUTION_H
#define BGEXECUTION_H

#include <unistd.h>
#include "shellState.h"

int execute_background_commands (BGJobManager* jm, ProcessManager* pm, char** args);
int execute_foreground_commands (char** args, ProcessManager* pm, FGJobManager* fm);

int fg_cmd (ProcessManager* pm, BGJobManager* jm, FGJobManager* fm, char** args);
int bg_cmd (ProcessManager* pm, BGJobManager* jm, char** args);

BGJob* bg_job_manager_find_job (BGJobManager* jm, int job_number);
BGJob* bg_job_manager_get_latest_job (BGJobManager* jm);

int is_bg_cmd (char** args);
void remove_bg_operator (char** args);

#endif