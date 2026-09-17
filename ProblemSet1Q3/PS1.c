#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>


/*ECE-357 Program set 1 question 3
Implementation of "kit" a cat-like command
Author: Jayden Chen

/*Intentional Behaviors
- -o and -b are not valid file names
- this program will not read files named "-"
*/

int copyTillEOF(int infd, int outfd, char *buf, int lim, char* ifilename, char* ofilename) //return -1 on read error, return -2 on write error, else return 0
{
    int r,w;
    while ((r = read(infd, buf, lim)) > 0){
        int writtenbytes = 0;
        while (writtenbytes < r){ //handles partial writes by looping till either error or writtenbytes = r
            w = write(outfd, buf, r);
            if (w < 0) { //handles write errors
                fprintf(stderr, "Error writing to %s: %s\n", ofilename, strerror(errno));
                return -2;
            }
            writtenbytes += w;
        }
        
    } 
    if (r < 0) {
        fprintf(stderr, "Error reading from %s: %s\n", ifilename, strerror(errno));
        return -1;
    }
    return 0;
}

int main(int argc, char **argv)
{
    char *buf;
    int bufsize;
    int outfd = STDOUT_FILENO;
    int infd, opt, i;
    char* ifilename;
    char* ofilename = "stdout";
    
    while ((opt = getopt(argc, argv, "ob:")) != -1) { //getopt to handle -o
        switch (opt) {
        case 'o':
            if(outfd != STDOUT_FILENO){
                fprintf(stderr, "Error opening %s as output file, only one output file is allowed\n", optarg);
                return -1;
            }
            
            outfd = open(optarg, O_WRONLY | O_CREAT | O_TRUNC, 0666);
            if (outfd < 0) {
                fprintf(stderr, "Error opening %s for writing: %s\n", optarg, strerror(errno));
                return -1;
            }
            ofilename = optarg;
            break;
        case 'b':
            bufsize = atoi(optarg);
            if(bufsize <= 0){
                fprintf(stderr, "Buffer size must be greated than 0");
                return -1;
            }
        default:
            fprintf(stderr, "Usage: kit [-o outfile] infile...\n");
            return -1;
        }
    }
    buf = malloc(bufsize); 
    if (buf == NULL){
        fprintf(stderr, "Error allocating buffer of size %d: %s\n", bufsize, strerror(errno));
    }

    for (i = optind; i < argc; i++) { //parse rest of args "-*"
        if (strcmp(argv[i], "-") == 0) {
            infd = STDIN_FILENO;
            ifilename = "stdin";
        }
        else {
            ifilename = argv[i];
            infd = open(argv[i], O_RDONLY);
            if (infd < 0) {
                fprintf(stderr, "Error opening %s for reading: %s\n", argv[i], strerror(errno));
                return -1;
            }
        }

        if (copyTillEOF(infd, outfd, buf, sizeof(buf), ifilename, ofilename) < 0) { // moved error printing to func
            free(buf);
            return -1;
        }

        if (infd != STDIN_FILENO) {
            if (close(infd) < 0) {
                fprintf(stderr, "Error closing %s: %s\n", argv[i], strerror(errno));
                return -1;
            }
        }
    }
    if (optind == argc) { //handles no args input
        ifilename = "stdin";
        if (copyTillEOF(STDIN_FILENO, outfd, buf, sizeof(buf), ifilename, ofilename) < 0) {
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

    free(buf);
    return 0;
}