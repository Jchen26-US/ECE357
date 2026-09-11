#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>

#define PROGRAM_NAME "kit"

/*ECE-357 Program set 1 question 3
Implementation of "kit" a cat-like command
Author: Jayden Chen

Current known issues
- output is writing chinese
- handling files named "-"
- handling case where two output files are defined
- handling file named "-o"
*/

int copyTillEOF(int infd, int outfd, char *buf, int lim) //return -1 on read error, return -2 on write error, else return 0
{
    int r,w;
    do {
        r = read(infd, buf, lim);
        if (r < 0) {
            return -1;
        }
        if (r > 0) {
            w = write(outfd, buf, r);
            if (w < 0) {
                return -2;
            }
        }
    } while (r > 0);
    return 0;
}

int main(int argc, char **argv)
{
    char buf[4096];

    int outfd = STDOUT_FILENO;
    int infd;
    int opt;
    int i;

    for (i = 0; i < argc; i++){
        fprintf(stdout, "arg %s \n", argv[i]);   
    }
    
    while ((opt = getopt(argc, argv, "o:")) != -1) { //getopt to handle -o
        switch (opt) {

        case 'o':
            outfd = open(optarg, O_WRONLY | O_CREAT | O_TRUNC, 0666);
            fprintf(stdout, "opened %s for writing\n", optarg); //testing print
            if (outfd < 0) {
                fprintf(stderr, "Error opening %s for writing: %s\n", optarg, strerror(errno));
                return -1;
            }
            break;
        default:
            fprintf(stderr, "Usage: kit [-o outfile] infile...\n");
            return -1;
        }
    }

    for (i = optind; i < argc; i++) { //parse rest of args "-*" except -o 
        fprintf(stdout, "opening %s as input\n", argv[i]);
        if (strcmp(argv[i], "-") == 0) {
            infd = STDIN_FILENO;
        }
        else {
            infd = open(argv[i], O_RDONLY);
            if (infd < 0) {
                fprintf(stderr, "Error opening %s for reading: %s\n", argv[i], strerror(errno));
                return -1;
            }
        }

        if (copyTillEOF(infd, outfd, buf, sizeof(buf)) < 0) {
            fprintf(stderr, "Error copying %s: %s\n", argv[i], strerror(errno));
            return -1;
        }

        if (infd != STDIN_FILENO) {
            if (close(infd) < 0) {
                fprintf(stderr, "Error closing %s: %s\n", argv[i], strerror(errno));
                return -1;
            }
        }
    }

    if (optind == argc) {
        if (copyTillEOF(STDIN_FILENO, outfd, buf, sizeof(buf)) < 0) {
            fprintf(stderr, "Error reading stdin: %s\n", strerror(errno));
            return -1;
        }
    }

    if (outfd != STDOUT_FILENO) {
        if (close(outfd) < 0) {
            fprintf(stderr, "Error closing output: %s\n",
                    strerror(errno));
            return -1;
        }
    }

    return 0;
}