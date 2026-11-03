#define _POSIX_C_SOURCE 200809L
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <limits.h>

volatile sig_atomic_t signalsHandled = 0;

void handleSignal(int sig){
    (void)sig;
    signalsHandled++;
}

int runReader(int fd){
    char buf[555];
    unsigned long long counts[556] = {0};
    unsigned long long total = 0;
    ssize_t r;
    int i;

    while ((r = read(fd, buf, sizeof(buf))) > 0) {
        counts[r]++;
        total += r;
    }
    if (r < 0) {
        fprintf(stderr, "Error reading from pipe: %s\n", strerror(errno));
        return -1;
    }
    printf("Read returned EOF\n");
    for (i = 1; i <= 555; i++) {
        if (counts[i] > 0)
            printf("%llu reads of %d bytes\n", counts[i], i);
    }
    printf("%llu total bytes read\n", total);
    return 0;
}

int runWriter(int fd, int readyfd, size_t size, long iterations, int restart){
    char *buf;
    unsigned long long *counts;
    unsigned long long interrupted = 0, total = 0;
    struct sigaction sa;
    ssize_t w;
    long i;
    size_t j;

    buf = malloc(size);
    counts = calloc(size + 1, sizeof(*counts));
    if (buf == NULL || counts == NULL) {
        fprintf(stderr, "Error allocating writer buffer: %s\n", strerror(errno));
        free(buf);
        free(counts);
        return -1;
    }
    if (counts == NULL) {
        fprintf(stderr, "Error allocating writer histogram: %s\n", strerror(errno));
        free(buf);
        free(counts);
        return -1;
    }
    memset(buf, 'A', size);

    sa.sa_handler = handleSignal;
    sa.sa_flags = restart ? SA_RESTART : 0;
    if (sigemptyset(&sa.sa_mask) < 0 || sigaction(SIGUSR1, &sa, NULL) < 0) {
        fprintf(stderr, "Error establishing SIGUSR1 handler: %s\n", strerror(errno));
        free(buf);
        free(counts);
        return -1;
    }

    // signal handler for writer is ready
    if (write(readyfd, "R", 1) != 1) {
        fprintf(stderr, "Error notifying parent: %s\n", strerror(errno));
        free(buf);
        free(counts);
        return -1;
    }
    if (close(readyfd) < 0) {
        fprintf(stderr, "Error closing readiness pipe: %s\n", strerror(errno));
        free(buf);
        free(counts);
        return -1;
    }

    for (i = 0; i < iterations; i++) {
        w = write(fd, buf, size);
        if (w < 0) {
            if (errno == EINTR) {
                interrupted++;
                continue;
            }
            int curErr = errno;
            sigaction(SIGUSR1, &(struct sigaction){.sa_handler = SIG_IGN}, NULL);
            fprintf(stderr, "Error writing to pipe: %s\n", strerror(curErr));
            free(buf);
            free(counts);
            return -1;
        }
        // Count short writes and continue w/o retrying
        counts[w]++;
        total += w;
    }

    // Prevent signals from interrupting stdio output.
    sa.sa_handler = SIG_IGN;
    sa.sa_flags = 0;
    if (sigaction(SIGUSR1, &sa, NULL) < 0) {
        fprintf(stderr, "Error ignoring SIGUSR1: %s\n", strerror(errno));
        free(buf);
        free(counts);
        return -1;
    }

    printf("%llu EINTRs\n", interrupted);
    printf("%d SIGUSR1 handled\n", (int)signalsHandled);
    for (j = 1; j <= size; j++) {
        if (counts[j] > 0)
            printf("%llu writes of %zu bytes\n", counts[j], j);
    }
    printf("%llu total bytes written\n", total);
    free(buf);
    free(counts);
    return 0;
}

int main(int argc, char **argv){
    int pfd[2]; 
    int ready[2]; //tracks whether read end [0] is ready and write end [1] is ready
    int status; //exit child statu
    int restart = 0;
    pid_t reader, writer, result;
    long size, iterations;
    char *end;
    char readyByte;
    unsigned long long signalsSent = 0;
    struct timespec delay = {0, 5000}; // 5 microseconds

    if (argc != 3 && argc != 4) {
        fprintf(stderr, "Usage: %s write_size iterations [restart]\n", argv[0]);
        return -1;
    }
    errno = 0;
    size = strtol(argv[1], &end, 10);
    if (errno || *end != '\0' || size <= 0 || size > INT_MAX) {
        fprintf(stderr, "Error: write size must be a positive integer <= INT_MAX\n");
        return -1;
    }
    errno = 0;
    iterations = strtol(argv[2], &end, 10);
    if (errno || *end != '\0' || iterations <= 0) {
        fprintf(stderr, "Error: iterations must be a positive integer\n");
        return -1;
    }
    if (argc == 4) {
        if (strcmp(argv[3], "restart") != 0) {
            fprintf(stderr, "Usage: %s write_size iterations [restart]\n", argv[0]);
            return -1;
        }
        restart = 1;
    }

    if (pipe(pfd) < 0 || pipe(ready) < 0) {
        fprintf(stderr, "Error creating pipe: %s\n", strerror(errno));
        return -1;
    }
    reader = fork();
    if (reader < 0) {
        fprintf(stderr, "Error forking reader: %s\n", strerror(errno));
        return -1;
    }
    if (reader == 0) {
        close(pfd[1]);
        close(ready[0]);
        close(ready[1]);
        status = runReader(pfd[0]);
        close(pfd[0]);
        return status < 0 ? 1 : 0;
    }

    writer = fork();
    if (writer < 0) {
        fprintf(stderr, "Error forking writer: %s\n", strerror(errno));
        return -1;
    }
    if (writer == 0) {
        close(pfd[0]);
        close(ready[0]);
        status = runWriter(pfd[1], ready[1], (size_t)size, iterations, restart);
        close(pfd[1]);
        return status < 0 ? 1 : 0;
    }

    close(pfd[0]);
    close(pfd[1]);
    close(ready[1]);
    if (read(ready[0], &readyByte, 1) != 1) {
        fprintf(stderr, "Error waiting for writer readiness: %s\n", strerror(errno));
        close(ready[0]);
        waitpid(writer, &status, 0);
        waitpid(reader, &status, 0);
        return -1;
    }
    close(ready[0]);

    for (;;) {
        result = waitpid(writer, &status, WNOHANG);
        if (result == writer) {
            printf("Writer is done\n");
            break;
        }
        if (result < 0) {
            fprintf(stderr, "Error waiting for writer: %s\n", strerror(errno));
            return -1;
        }
        if (kill(writer, SIGUSR1) < 0) {
            if (errno == ESRCH) {
                printf("kill returned ESRCH, writer is done\n");
                waitpid(writer, &status, 0);
                break;
            }
            fprintf(stderr, "Error signaling writer: %s\n", strerror(errno));
            return -1;
        }
        signalsSent++;
        nanosleep(&delay, NULL);
    }
    printf("%llu SIGUSR1 sent\n", signalsSent);
    if (waitpid(reader, &status, 0) < 0) {
        fprintf(stderr, "Error waiting for reader: %s\n", strerror(errno));
        return -1;
    }
    return 0;
}
