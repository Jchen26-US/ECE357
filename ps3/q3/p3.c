#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>  //is an error on windows systems
#include <sys/resource.h> //is an error on windows systems
#include <time.h>
#include <errno.h>

#define MAXARGS 256

static double seconds(struct timespec t) {
    return t.tv_sec + t.tv_nsec / 1000000000.0;
}

int main(int argc, char **argv) {
    FILE *input = stdin;
    char *line = NULL;
    size_t capacity = 0;
    int last_status = 0;

    if (argc > 2) {
        fprintf(stderr, "Usage: %s [script.sh]\n", argv[0]);
        return 1;
    }
    if (argc == 2 && !(input = fopen(argv[1], "r"))) {
            fprintf(stderr, "Error opening script %s: %s\n",
                argv[1], strerror(errno));
        perror(argv[1]);
        return 1;
    }

    
    while (getline(&line, &capacity, input) != -1) { //read line ignoring blank lines and comments
        char *args[MAXARGS];
        char *files[3] = {NULL, NULL, NULL};
        int modes[3] = {0, 0, 0};  // 0=unused, 1=truncate/read, 2=append
        int count = 0, redirecting = 0, bad = 0;
        char *token = strtok(line, " \t\r\n");

        if (!token || token[0] == '#')
            continue;

        while (token) {
            int fd = -1, mode = 1;
            char *name = NULL;
            //Detecting redirections
            if (strncmp(token, "2>>", 3) == 0) { fd = 2; mode = 2; name = token + 3; }
            else if (strncmp(token, ">>", 2) == 0) { fd = 1; mode = 2; name = token + 2; }
            else if (strncmp(token, "2>", 2) == 0) { fd = 2; name = token + 2; }
            else if (token[0] == '>') { fd = 1; name = token + 1; }
            else if (token[0] == '<') { fd = 0; name = token + 1; }

            if (fd >= 0) {
                redirecting = 1;
                if (!*name) {
                    fprintf(stderr, "Error parsing redirection %s: Missing filename\n", token);
                    bad = 1;
                    break;
                }
                if (files[fd]) {
                    fprintf(stderr, "Error redirecting %s: File descriptor %d already redirected to %s\n", token, fd, files[fd]);
                    bad = 1;
                    break;
                }
                files[fd] = name;
                modes[fd] = mode;
            } 
            else if (redirecting) {
                fprintf(stderr, "Error parsing argument %s: Argument appears after redirection\n", token);
                bad = 1;
                break;
            }
            else if (count >= MAXARGS - 1) {
                fprintf(stderr, "Error parsing argument %s: Too many arguments\n", token);
                bad = 1;
                break;
            }
            else {
                args[count++] = token;
            }
            token = strtok(NULL, " \t\r\n");
        }
        args[count] = NULL;
        if (bad) {
            last_status = 1;
            continue;
        }

        if (count == 0) {
            fprintf(stderr, "Invalid command syntax: Missing command\n");
            last_status = 1;
            continue;
        }

        if (strcmp(args[0], "exit") == 0) {
            if (count > 1) {
                char *end;
                long value = strtol(args[1], &end, 10);
                if (*end || end == args[1]) {
                    fprintf(stderr,
                        "Error: Exit status must be an integer\n");
                    last_status = 1;
                    continue;
                }
                last_status = (unsigned char)value;
            }
            break;
        }
        if (strcmp(args[0], "cd") == 0) {
            char *dir = count > 1 ? args[1] : getenv("HOME");
            if (!dir) {
                fprintf(stderr, "Error changing directory: HOME is not set\n");
                last_status = 1;
            }
            else if (chdir(dir) < 0) {
                fprintf(stderr, "Error changing directory: %s\n",
                    strerror(errno));
                last_status = 1;
            }
            continue;
        }
        if (strcmp(args[0], "pwd") == 0) {
            char *cwd = getcwd(NULL, 0);
            if (!cwd) {
                fprintf(stderr, "Error getting current directory: %s\n",
                    strerror(errno));
                last_status = 1; 
            }
            else {
                puts(cwd);
                free(cwd); 
            }
            continue;
        }

        struct timespec start, end;
        if (clock_gettime(CLOCK_MONOTONIC, &start) < 0) {
            fprintf(stderr, "Error getting start time: %s\n",
                    strerror(errno));
            last_status = 1;
            continue;
        }
        pid_t pid = fork();
        if (pid < 0) {
            fprintf(stderr, "Error forking process: %s\n",
                    strerror(errno));
            last_status = 1;
            continue;
        }
        if (pid == 0) {
            //Close script file in child
            if (input != stdin){
                if (close(fileno(input)) < 0) {
                    fprintf(stderr,
                            "Error closing script file %s: %s\n",
                            argv[1], strerror(errno));
                    _exit(1);
                }
            }
            
            for (int fd = 0; fd < 3; fd++) {
                if (!files[fd])
                    continue;
                int flags = fd == 0 ? O_RDONLY : O_WRONLY | O_CREAT |
                            (modes[fd] == 2 ? O_APPEND : O_TRUNC);

                int opened = open(files[fd], flags, 0666);

                if (opened < 0) {
                    fprintf(stderr, "Error opening %s: %s\n", files[fd], strerror(errno));
                    _exit(1);
                }

                if (dup2(opened, fd) < 0) {
                    fprintf(stderr, "Error redirecting %s: %s\n", files[fd], strerror(errno));
                    if (opened != fd)
                        close(opened);
                    _exit(1);
                }

                if (opened != fd) {
                    if (close(opened) < 0) {
                        fprintf(stderr, "Error closing file: %s\n",
                                strerror(errno));
                        _exit(1);
                    }
                }
            }
            execvp(args[0], args);
            fprintf(stderr, "Error executing %s: %s\n", args[0], strerror(errno));
            _exit(127);
        }
        //parent waiting for child
        int status;
        struct rusage usage;
        if (wait4(pid, &status, 0, &usage) < 0) {
            fprintf(stderr, "Error waiting for process %ld (%s): %s\n", (long)pid, args[0], strerror(errno));
            last_status = 1;
            continue;
        }
        clock_gettime(CLOCK_MONOTONIC, &end);

        if (WIFEXITED(status)) {
            last_status = WEXITSTATUS(status);
            fprintf(stderr, "Child process %ld exited with status %d\n", (long)pid, last_status);
        } else if (WIFSIGNALED(status)) {
            int sig = WTERMSIG(status);
            last_status = 128 + sig;
            fprintf(stderr, "Child process %ld killed by signal %d\n", (long)pid, sig);
        }
        double real = seconds(end) - seconds(start);
        double user = usage.ru_utime.tv_sec + usage.ru_utime.tv_usec / 1e6;
        double sys = usage.ru_stime.tv_sec + usage.ru_stime.tv_usec / 1e6;
        fprintf(stderr, "Real: %.3fs User: %.3fs Sys: %.3fs\n", real, user, sys);
    }
    if (ferror(input)) {
        fprintf(stderr, "Error reading input: %s\n",
                strerror(errno));
        last_status = 1;
    }
    free(line);
    if (input != stdin) fclose(input);
    return last_status;
}
