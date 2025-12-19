#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pwd.h>

#include "../include/activities.h"
#include "../include/bgExecution.h"
#include "../include/cfgParser.h"
#include "../include/hop.h"
#include "../include/reveal.h"
#include "../include/keyboardShortcuts.h"
#include "../include/log.h"
#include "../include/redirection.h"
#include "../include/sequential.h"
#include "../include/shellState.h"
#include "../include/shellInput.h"

// typedef struct {
//     Hopstate hop_state;
// } ShellState;

// int process_command(char* command_line, ShellState* shell_state, char* home_dir, int* should_exit);

ShellState* init_shell_state () {
    ShellState* new_shellstate = malloc(sizeof(ShellState));
    
    new_shellstate->hop_state = init_hop_state();
    new_shellstate->bg_job_manager = bg_job_manager_create();
    new_shellstate->process_manager = process_manager_create();
    new_shellstate->fg_job_manager = fg_job_manager_init();
    new_shellstate->should_exit = 0;

    return new_shellstate;
}

void initialise_shell(char** username, char** systemname, char** home_dir) {
    // printf("in function 1\n");

    struct passwd *pw = getpwuid(getuid());
    if (pw) *username = strdup(pw->pw_name);
    else printf("Username not found!");

    *systemname = malloc(256);
    int sys_flag = gethostname(*systemname, 256);
    if (sys_flag != 0) printf("System name not found!");

    *home_dir = getcwd(NULL, 0);
    if (*home_dir == NULL) printf("Home directory not found!");

    // printf("done with function 1\n");
}

char* current_directory(char* home_dir) {
    // printf("in function 3\n");
    char* cwd = getcwd(NULL, 0);
    if (strstr(cwd, home_dir) == cwd) {
        char* relative = cwd + strlen(home_dir);
        char* result = malloc(strlen(relative) + 2);
        sprintf(result, "~%s", relative);
        free(cwd);
        return result;
    }
    // printf("in function 3\n");

    return cwd;
}

void shell(char* username, char* systemname, char* home_dir) {
    char* line = NULL;
    size_t buffer_size = 0;
    ssize_t chars_read;
    ShellState* shell_state = init_shell_state();
    int command_validity = 0;
    int should_exit = 0;

    install_signal_handlers(shell_state);

    while(1) {
        bg_job_manager_check_completed(shell_state->bg_job_manager, shell_state->process_manager);

        // printf("<\033[1;35m%s@%s\033[0m:\033[1;37m%s\033[0m> ", username, systemname, current_directory(home_dir));
        printf("<%s@%s:%s> ", username, systemname, current_directory(home_dir));
        fflush(stdout);

        if (line) {
            free(line);
            line = NULL;
        }
        buffer_size = 0;
        command_validity = 0;

        chars_read = getline(&line, &buffer_size, stdin);

        if (chars_read == -1) {
            if (feof(stdin)) cleanup_and_exit(shell_state, 0);
            else {
                perror("getline");
                break;
            }
        }

        if (chars_read > 0 && line[chars_read - 1] == '\n') line[chars_read - 1] = '\0';

        if (strlen(line) == 0) continue;

        if (!check_input_validity(line)){
            printf("Invalid Syntax!\n");
            continue;
        }

        command_validity = process_command(line, shell_state, home_dir, &should_exit);
        if (command_validity == 0 && strncmp(line, "log", 3) != 0) write_to_log(line);
        if (should_exit) break;
    }

    if (line) free(line);
    shell_cleanup(shell_state);
    // cleanup_hop_state(&shell_state.hop_state);
}

int execute_builtin_cmd (char** tokens, int token_count, Hopstate* hop_state, char* home_dir, ShellState* shell_state, int* should_exit) {
    if (token_count == 0) return -1;
    char* command = tokens[0];

    if (strcmp(command, "hop") == 0) return execute_hop_command(hop_state, tokens, &home_dir);
    else if (strcmp(command, "reveal") == 0) {
        char reveal_command[1024] = {0};
        for (int i = 0; i < token_count && tokens[i] != NULL; i++) {
            if (i>0) strcat(reveal_command, " ");
            strcat(reveal_command, tokens[i]);
        }
        return parse_reveal_command(reveal_command, hop_state, &home_dir);
    }
    else if (strcmp(command, "activities") == 0) {
        if (token_count > 1) {
            printf("Error: activities command takes no arguments.\n");
            return -1;
        }
        bg_job_manager_check_completed(shell_state->bg_job_manager, shell_state->process_manager);
        process_manager_list_activities(shell_state->process_manager);
        return 0;
    }
    else if (strcmp(command, "log") == 0) {
        char log_command[1024] = {0};
        for (int i = 0; i < token_count && tokens[i] != NULL; i++) {
            if (i>0) strcat(log_command, " ");
            strcat(log_command, tokens[i]);
        }
        return parse_log_command(log_command, shell_state, &home_dir, should_exit);
    }
    else if (strcmp(command, "cd") == 0) {
        char* path = (token_count > 1) ? tokens[1] : home_dir;
        if (chdir(path) != 0) {
            perror("cd");
            return -1;
        }
        return 0;
    }
    else if (strcmp(command, "exit") == 0) {
        if (token_count > 1) {
            fprintf(stderr, "Error: Too many aaarguments.\n");
            return -1;
        }
        *should_exit = 1;
        return 0;
    }
    else if (strcmp(command, "fg") == 0) {
        if (token_count > 2) {
            fprintf(stderr, "Error: Too many arguments for fg command.\n");
            return -1;
        }
        return fg_cmd(shell_state->process_manager, shell_state->bg_job_manager, shell_state->fg_job_manager, tokens);
    }
    else if (strcmp(command, "bg") == 0) {
        if (token_count > 2) {
            fprintf(stderr, "Error: Too many arguments for bg command.\n");
            return -1;
        }
        return bg_cmd(shell_state->process_manager, shell_state->bg_job_manager, tokens);
    }

    return -1;
}

int process_command(char* command_line, ShellState* shell_state, char* home_dir, int* should_exit) {
    char* token;
    char* line_copy = strdup(command_line);

    if (line_copy == NULL) return -1;

    //SEQUENTIAL EXECUTION
    if (strstr(command_line, ";") != NULL) {
        int result = parse_sequential_cmd(command_line, shell_state, home_dir, should_exit);
        free(line_copy);
        return (result == 0) ? 0 : -1;
    }

    //PIPELINE COMMANDS
    if (strstr(command_line, "|") != NULL || strstr(command_line, ">") != NULL || strstr(command_line, "<") != NULL || strstr(command_line, ">>") != NULL) {
        int result = parse_redirection_cmd(command_line, &shell_state->hop_state, home_dir, shell_state, should_exit);
        free(line_copy);
        return (result == 0) ? 0 : -1;
    }

    char* tokens[256] = {NULL};
    int token_count = 0;
    
    token = strtok(line_copy, " \t\n");
    while (token && token_count < 255) {
        tokens[token_count++] = token;
        token = strtok(NULL, " \t\n");
    }

    if (token_count == 0){
        free(line_copy);
        return -1;
    }

    if (token_count > 0 && strcmp(tokens[0], "activities") == 0) {
        int result = execute_builtin_cmd(tokens, token_count, &shell_state->hop_state, home_dir, shell_state, should_exit);
        free(line_copy);
        return result;
    }

    //BACKGROUND EXECUTION
    int bg = 0;
    for (int i = 0; i < token_count; i++) {
        if (strcmp(tokens[i], "&") == 0) {
            bg = 1;
            tokens[i] = NULL;
            break;
        }
    }
    // if (strstr(command_line, "&") != NULL) bg = 1;

    int builtin_result = execute_builtin_cmd(tokens, token_count, &shell_state->hop_state, home_dir, shell_state, should_exit);
    if (builtin_result != -1) {
        free(line_copy);
        return (builtin_result == 0) ? 0 : -1;
    }

    char** command_arr = malloc((token_count+1) * sizeof(char*));
    if (!command_arr) {
        free(line_copy);
        return -1;
    }

    for (int i = 0; i < token_count; i++) command_arr[i] = tokens[i];
    command_arr[token_count] = NULL;

    int result;
    if (bg) {
        // remove_bg_operator(command_arr);
        result = execute_background_commands(shell_state->bg_job_manager, shell_state->process_manager, command_arr);
    }
    else result = execute_foreground_commands(command_arr, shell_state->process_manager, shell_state->fg_job_manager);

    free(command_arr);

    free(line_copy);
    return (result == 0) ? 0 : -1;
}

void shell_cleanup(ShellState* state) {
    if (state) {
        bg_job_manager_cleanup(state->bg_job_manager);
        process_manager_cleanup(state->process_manager);
        free(state);
    }
}

void cleanup(char* username, char* systemname, char* home_dir) {
    free(username);
    free(systemname);
    free(home_dir);
}

int main() {
    char* username = NULL;
    char* systemname = NULL;
    char* home_dir = NULL;

    initialise_shell(&username, &systemname, &home_dir);
    shell(username, systemname, home_dir);

    cleanup(username, systemname, home_dir);

    return 0;
}







    // //HOP COMMAND
    // if (strcmp(token, "hop") == 0) execute_hop_command(&shell_state->hop_state, args, &home_dir);

    // //REVEAL COMMAND
    // else if (strcmp(token, "reveal") == 0) {
    //     char* path = NULL;
    //     char* flags[256] = {NULL};
    //     int flag_count = 0;

    //     token = strtok(NULL, " \t\n");
    //     while (token != NULL && flag_count < 255) {
    //         if (strncmp(token, "-", 1) == 0) flags[flag_count++] = token;
    //         else {
    //             if (path == NULL) path = strdup(token);
    //             else {
    //                 char* new_path = malloc(strlen(path) + strlen(token) + 2);
    //                 sprintf(new_path, "%s %s", path, token);
    //                 free(path);
    //                 path = new_path;
    //             }
    //         }
    //         token = strtok(NULL, " \t\n");
    //     }
    //     execute_reveal_command(&shell_state->hop_state, flags, path, &home_dir);

    //     if (path) free(path);
    // }

    // //LOG COMMAND
    // else if (strcmp(token, "log") == 0) {
    //     char* arg = strtok(NULL, " \t\n");
    //     if (arg == NULL) print_log();
    //     else if (strcmp(arg, "purge") == 0) {
    //         char* extra = strtok(NULL, " \t\n");
    //         if (extra != NULL){
    //             fprintf(stderr, "Invalid syntxx! Purge doesn't take additional arguments.\n");
    //             valid = 0;
    //         }
    //         else delete_from_log();
    //     }
    //     else if (strcmp(arg, "execute") == 0) {
    //         char* index = strtok(NULL, " \t\n");
    //         if (index == NULL){
    //             fprintf(stderr, "Invalid Syntax! Expected an integer.");
    //             valid = 0;
    //         }
    //         else {
    //             char* extra = strtok(NULL, " \t\n");
    //             if (extra != NULL){
    //                 fprintf(stderr, "Too many arguments\n");
    //                 valid = 0;
    //             }
    //             else {
    //                 char* endptr;
    //                 long ind = strtol(index, &endptr, 10);

    //                 if (*endptr != '\0'){
    //                     fprintf(stderr, "Error: '%s' is not a valid token\n", index);
    //                     valid = 0;
    //                 }
    //                 else if (ind < 0){
    //                     fprintf(stderr, "Error: Index must be non-negative\n");
    //                     valid = 0;
    //                 }
    //                 else {
    //                     char* saved_command = execute_command_from_log((int)ind);
    //                     if (saved_command != NULL) {
    //                         process_command(saved_command, hop_state, shell_state, home_dir, should_exit);
    //                         free(saved_command);
    //                     }
    //                     else valid = 0;
    //                 }
    //             }
    //         }
    //     }
    //     else{
    //         fprintf(stderr, "Invalid Syntax! Unknown condition.\n");
    //         valid = 0;
    //     }
    // }

    // //OTHER COMMANDS
    // else if (strcmp(token, "cd") == 0) {
    //     char* path = strtok(NULL, " \t\n");
    //     if (path == NULL) path = home_dir;
    //     if (chdir(path) != 0){
    //         perror("cd");
    //         valid = 0;
    //     }
    // }

    // else if (strcmp(token, "exit") == 0) {
    //     char* extra = strtok(NULL, " \t\n");
    //     if (extra != NULL) {
    //         fprintf(stderr, "Error: Too many arguments\n");
    //         valid = 0;
    //     }
    //     else *should_exit = 1;
    // }
    
    

    // free(line_copy);
    // return valid;