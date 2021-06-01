#include <stdio.h>
#include <unistd.h>

void foo(int a)
{
    printf("%d\n",a);
}

int main(int argc,char *argv[])
{
    for (int i = 0;i < argc;i++) {
        printf("%s\n",argv[i]);
    }

    for (int i = 0;i < 100000000;i++) {
        foo(i);
        sleep(1);
    }

    return 0;
}