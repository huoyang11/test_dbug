#include <stdio.h>
#include <unistd.h>

int main(int argc,char *argv[])
{
    write(STDOUT_FILENO,"aaaa\n",5);
    write(STDOUT_FILENO,"bbbb\n",5);
    write(STDOUT_FILENO,"cccc\n",5);

    return 0;
}