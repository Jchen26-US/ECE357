#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

void f1()
{
    static int i = 10;
    printf("%d\n",i);
    i++;
}

int main(int argc, char **argv)
{
    int ws = -1;
    f1();
    if (fork() == 0)
        f1();
    f1();
    wait(&ws);
    return (ws >> 8) & 255;
}
