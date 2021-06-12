#include <stdio.h>
#include <unistd.h>

int add(int a,int b)
{
    return a + b;
}

int sub(int a,int b)
{
    return a - b;
}

int main(int argc,char *argv[])
{
    printf("main = %p\n",(void *)main);
    printf("add  = %p\n",(void *)add);
    printf("sub  = %p\n",(void *)sub);

    sleep(2);

    return 0;
}