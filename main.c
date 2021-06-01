#include <stdio.h>
#include "command.h"

int main(int argc,char *argv[])
{
    struct dbug_struct pro = {0};
    init_dbug(&pro);

    while(1) {
        play_command(&pro,get_command(&pro));
    }

    uninit_dbug(&pro);
    return 0;
}