#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int print_int(int a)
{
    printf("%d\n",a);

    return 0;
}

int main(int argc,char *argv[])
{
    printf("print_int = %p\n",(void *)print_int);

    printf("pid = %d\n",getpid());
    sleep(10);

    for (int i = 0;i < 10;i++) {
        print_int(i);
    }

    return 0;
}