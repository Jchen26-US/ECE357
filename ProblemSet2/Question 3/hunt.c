#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>


#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <limits.h>

static const char *instr;
static struct stat targetFile;
static int targetFd;

int compare_contents(const char *path){

}

int walk(const char *dir){ //recursive function to traverse the directory
    DIR *dirp = opendir(dir);
    if (!dirp){
        fprintf(stderr, "Erorr occured while opening directory: %s, strerr: %s\n", dir, strerror(errno));
        return -1;
    }
    struct dirent *de;
    while(1){
        errno = 0;
        de = readdir(dirp);
        if(!de){
            if (errno != 0){
                fprintf(stderr, "Error occured while reading directory entry in directory: %s, strerr: %s \n", dir, strerror(errno)); //only for testing purposes
                return -1;
            }
            else{
                //errno == 0
                //reached the end of directory
                return 0;
            }
        }
        //skip . and .. entries
        if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0){
            continue;
        }

        //current path
        char path[PATH_MAX];
        int len = snprintf(path, sizeof(path), "%s/%s", dir, de->d_name);
        if(len < 0 || (size_t)len >= sizeof(path)){
            fprintf(stderr, "%s: path was too long: %s/%s\n", instr, dir, de->d_name);
            continue;
        }

        //handling symbolic links without following
        struct stat st;
        if (lstat(path, &st) < 0){
            fprintf(stderr, "error during statting file at path: %s, strerr: %s\n", path, strerror(errno));
            continue;
        }
        if(S_ISLNK(st.st_mode)){
            struct stat rst;
            if(stat(path, &rst) < 0){
                fprintf(stderr, "dangling link at path: %s, strerr: %s\n", path, strerror(errno));
                continue;
            }
            if(rst.st_dev == targetFile.st_dev && rst.st_ino == targetFile.st_ino){
                fprintf(stdout, "%s\tSYMLINK Resolves to target\n", path);
                continue;
            }
            if(rst.st_size != targetFile.st_size){
                continue;
            }
            if(compare_contents(path)){
                char linkbuf[PATH_MAX +1];
                ssize_t n = readlink(path, linkbuf, sizeof(linkbuf) -1);
                if(n < 0){
                    continue;
                }
                linkbuf[n] = '\0';
                fprintf(stdout, "%s\tSYMLINK (%s) RESOLVES TO DUPLICATE (nlink=%ju, dev=0x%jx, ino=%ju)\n",path, linkbuf, (uintmax_t)rst.st_nlink,(uintmax_t)rst.st_dev, (uintmax_t)rst.st_ino);
            }
        }
        //handling regular files
        if (S_ISREG(st.st_mode)){
            if(st.st_dev == targetFile.st_dev && st.st_ino == targetFile.st_ino){
                fprintf(stderr, "Found match for target file at path: %s\n", path);
            }
            if(st.st_size != targetFile.st_size){
                continue;
            }
            if(compare_contants(path)){
                fprintf(stdout, "%s\tDUPLICATE OF TARGET (nlink = %ju, dev = 0x%jx, ino=%ju)\n", path, (uintmax_t)st.st_nlink, (uintmax_t)st.st_dev, (uintmax_t)st.st_ino);
            }
        }
        else if(S_ISDIR(st.st_mode)){
            walk(path);
        }
        
    }
}

int main(int argc, char **argv){
    instr = argv[0];
    

    if(argc != 3){
        fprintf(stderr, "insufficient arguments, usage: %s target startdir\n", instr);
        return -1;
    }

    if (stat(argv[1], &targetFile) < 0){
        fprintf(stderr, "error while running stat on target: %s, strerr: %s\n", argv[1], strerror(errno));
        return -1;
    }

    if(!S_ISREG(targetFile.st_mode)){
        fprintf(stderr, "%s is not a regular file\n", argv[1]);
        return -1;
    }

    if((targetFd = open(argv[1], O_RDONLY)) < 0){
        fprintf(stderr, "Error opening target: %s, strerr: %s\n", argv[1], strerror(errno));
    }

    walk(argv[2]);

    close(targetFd);
    return 0;
}