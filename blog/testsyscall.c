#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

int main(int argc,char *argv[])
{
    struct stat statbuf;
    stat("test", &statbuf);

    return 0;
}