#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

#include "../include/shellInput.h"
#include "../include/redirection.h"
#include "../include/hop.h"
#include "../include/reveal.h"
#include "../include/log.h"


// int execute_builtin_cmd (char** tokens, int token_count, Hopstate* hop_state, char* home_dir, ShellState* shell_state, int* should_exit) {
//     if (token_count == 0) return -1;
//     char* command = tokens[0];

//     if (strcmp(command, "hop") == 0) return execute_hop_command(hop_state, tokens, &home_dir);
//     else if (strcmp(command, "reveal") == 0) return parse_reveal_command(command, hop_state, &home_dir);
//     else if (strcmp(command, "log") == 0) {
//         char log_command[1024] = {0};
//         for (int i = 0; i < token_count && tokens[i] != NULL; i++) {
//             if (i>0) strcat(log_command, " ");
//             strcat(log_command, tokens[i]);
//         }
//         return parse_log_command(command, shell_state, &home_dir, should_exit);
//     }
//     else if (strcmp(command, "cd") == 0) {
//         char* path = (token_count > 1) ? tokens[1] : home_dir;
//         if (chdir(path) != 0) {
//             perror("cd");
//             return -1;
//         }
//         return 0;
//     }
//     else if (strcmp(command, "exit") == 0) {
//         if (token_count > 1) {
//             fprintf(stderr, "Error: Too many aaarguments.\n");
//             return -1;
//         }
//         *should_exit = 1;
//         return 0;
//     }

//     return -1;
// }

int process_redirection (char** tokens, int token_count) {
    char* input_file = NULL;
    char* output_file = NULL;
    int append_mode = 0;
    int new_token_count = token_count;

    for (int i = 0; i < new_token_count; i++) {
        if (tokens[i] == NULL) continue;

        if (strcmp(tokens[i], "<") == 0) {
            if (i+1 >= new_token_count || tokens[i+1] == NULL) {
                fprintf(stderr, "Syntax error: no file specified after <\n");
                return -1;
            }
            input_file = tokens[i+1];
            tokens[i] = NULL;
            tokens[i+1] = NULL;
            i++;
        }
        else if (strcmp(tokens[i], ">") == 0) {
            if (i+1 >= new_token_count || tokens[i+1] == NULL) {
                fprintf(stderr, "Syntax error: no file specified after >\n");
                return -1;
            }
            output_file = tokens[i+1];
            append_mode = 0;
            tokens[i] = NULL;
            tokens[i+1] = NULL;
            i++;
        }
        else if (strcmp(tokens[i], ">>") == 0) {
            if (i+1 >= new_token_count || tokens[i+1] == NULL) {
                fprintf(stderr, "Syntax error: no file specified after >>\n");
                return -1;
            }
            output_file = tokens[i+1];
            append_mode = 1;
            tokens[i] = NULL;
            tokens[i+1] = NULL;
            i++;
        }
    }

    if (input_file != NULL) if (handle_input_redirection(input_file) == -1) return -1;
    if (output_file != NULL) if (handle_output_redirection(output_file, append_mode) == -1) return -1;

    return 0;
}

int execute_pipeline(char*** commands, int n, Hopstate* hop_state, char* home_dir, ShellState* shell_state, int* should_exit) {
    if (n == 0) return 0;
    if (n == 1) return execute_one_cmd(commands[0], hop_state, home_dir, shell_state, should_exit);

    int status = 0;
    int pipedfs[2 * (n-1)];
    pid_t pids[n];

    for (int i = 0; i < n-1; i++) {
        if (pipe(pipedfs+2*i) == -1) {
            perror("pipe creation failed");
            return -1;
        }
    }

    for (int i = 0; i<n; i++) {
        pids[i] = fork();
        if(pids[i] == -1) {
            perror("fork failed");
            for (int j = 0; j < (n-1)*2; j++) close(pipedfs[j]);

            return -1;
        }

        if (pids[i] == 0) {
            if (i > 0) {
                if (dup2(pipedfs[(i-1)*2], STDIN_FILENO) == -1) {
                    perror("dup2 pipe input failed");
                    exit(EXIT_FAILURE);
                }
            }

            if (i < n-1) {
                if (dup2(pipedfs[i*2+1], STDOUT_FILENO) == -1) {
                    perror("dup2 pipe output failed");
                    exit(EXIT_FAILURE);
                }
            }

            for (int j = 0; j < 2*(n-1); j++) close(pipedfs[j]);

            int token_count = 0;
            while(commands[i][token_count]) token_count++;
            int builtin_result = execute_builtin_cmd(commands[i], token_count, hop_state, home_dir, shell_state, should_exit);
            if (builtin_result != -1) exit(builtin_result);

            if (process_redirection(commands[i], token_count) == -1) exit(EXIT_FAILURE);

            if (execvp(commands[i][0], commands[i]) == -1) {
                fprintf(stderr, "%s: command not found\n");
                exit(127);
            }
            perror("execvp failed");
            exit(EXIT_FAILURE);
        }
    }

    for (int i = 0; i<2*(n-1); i++) close(pipedfs[i]);

    for (int i = 0; i<n; i++) {
        waitpid(pids[i], &status, 0);
        if (WIFEXITED(status) && WEXITSTATUS(status) != 0) status = WEXITSTATUS(status);
    }

    return status;
}

int execute_one_cmd(char** command, Hopstate* hop_state, char* home_dir, ShellState* shell_state, int* should_exit) {
    int token_count = 0;
    while (command[token_count]) token_count++;

    int builtin_result = execute_builtin_cmd(command, token_count, hop_state, home_dir, shell_state, should_exit);
    if (builtin_result != -1) return builtin_result;
    
    pid_t pid = fork();

    if (pid == -1) {
        perror("fork failed");
        return -1;
    }

    if (pid == 0) {
        int token_count = 0;
        while(command[token_count]) token_count++;

        if (process_redirection(command, token_count) == -1) exit(EXIT_FAILURE);

        execvp(command[0], command);
        perror("execvp failed");
        exit(EXIT_FAILURE);
    }

    else {
        int status;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status)) return WEXITSTATUS(status);

        return -1;
    }
}

int parse_redirection_cmd (char* command_line, Hopstate* hop_state, char* home_dir, ShellState* shell_state, int* should_exit) {
    char* cmd_str = strdup(command_line);
    int number_of_commands = 1;
    for (char *p = cmd_str; *p; p++) if (*p == '|') number_of_commands++;

    char*** commands = malloc((number_of_commands+1) * sizeof(char**));
    if (!commands) {
        perror("malloc failed");
        free(cmd_str);
        return -1;
    }

    char *saveptr1, *saveptr2;
    
    int cmd_index = 0;
    char* cmd_token = strtok_r(cmd_str, "|", &saveptr1);
    while (cmd_token) {
        char** tokens = malloc(64 * sizeof(char*));
        if (!tokens) {
            perror("malloc failed");
            for (int i = 0; i < cmd_index; i++) free(commands[i]);
            free(commands);
            free(cmd_str);
            return -1;
        }

        int token_index = 0;
        char* arg = strtok_r(cmd_token, " \t\n", &saveptr2);
        while (arg != NULL && token_index < 63) {
            tokens[token_index++] = arg;
            arg = strtok_r(NULL, " \t\n", &saveptr2);
        }
        tokens[token_index] = NULL;
        commands[cmd_index++] = tokens;
        cmd_token = strtok_r(NULL, "|", &saveptr1);
    }
    commands[cmd_index] = NULL;

    int result = execute_pipeline(commands, number_of_commands, hop_state, home_dir, shell_state, should_exit);

    for (int i = 0; i < number_of_commands; i++) free(commands[i]);
    free(commands);
    free(cmd_str);

    return result;
}

int handle_input_redirection (char* filename) {
    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        fprintf(stderr, "No such file/directory: %s\n");
        return -1;
    }

    if (dup2(fd, STDIN_FILENO) == -1) {
        perror("dup2 input redirection failed");
        close(fd);
        return -1;
    }

    close(fd);
    return 0;
}

int handle_output_redirection (char* filename, int append_flag) {
    int flags = O_WRONLY | O_CREAT;
    if (append_flag) {
        flags |= O_APPEND;
    } else {
        flags |= O_TRUNC;
    }

    int fd = open(filename, flags, 0644);
    if (fd == -1) {
        return -1;
    }

    if (dup2(fd, STDOUT_FILENO) == -1) {
        perror("dup2 output redirection failed");
        close(fd);
        return -1;
    }

    close(fd);
    return 0;
}