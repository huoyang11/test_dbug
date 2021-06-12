#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>

#define BUFSIZE 1024
#define str2uint64(str,num) sscanf(str,"%lx",&num)

uint64_t get_pid_base(pid_t pid)
{
    char buf[BUFSIZE] = {0};
    char *pro_maps_path = buf;

    // open /proc/pid/maps
    pro_maps_path += sprintf(pro_maps_path,"%s","/proc");
    pro_maps_path += sprintf(pro_maps_path,"/%d",pid);
    sprintf(pro_maps_path,"/%s","maps");
    FILE *fp = fopen(buf,"rb");
    if (!fp) {
        perror("open file err");
        return -1;
    }

    // read /proc/pid/exe
    memset(buf,0,BUFSIZE);
    char *pro_exe_path = buf;
    pro_exe_path += sprintf(pro_exe_path,"%s","/proc");
    pro_exe_path += sprintf(pro_exe_path,"/%d",pid);
    sprintf(pro_exe_path,"/%s","exe");
    char target[100] = {0};
    int target_len = readlink(buf,target,100);
    target[target_len] = 0;

    memset(buf,0,BUFSIZE);
    char *pro_addr = buf;
    char *pro_maps = buf + 100;
    char *pro_name = pro_maps + 100;
    char *p = pro_name + 256;

    char data[512] = {0};
    while (!feof(fp)) {      
        fgets(data,sizeof(data),fp);

        sscanf(data,"%[^ ] %[^ ] %[^ ] %[^ ] %[^ ] %[^ ]",pro_addr,p,pro_maps,p,p,pro_name);
        //printf("pro_addr %s pro_maps %s pro_name %s --> %d %d\n",pro_addr,pro_maps,pro_name,memcmp(pro_name,target,target_len-1),memcmp(pro_maps,"00000000",8));
        if (memcmp(pro_name,target,target_len-1) == 0 && memcmp(pro_maps,"00000000",8) == 0) {
            fclose(fp);
            memset(p,0,10);
            sscanf(pro_addr,"%[^-]",p);
            uint64_t num = 0;
            str2uint64(p,num);
            return num;
        }
        memset(data,0,sizeof(data));
    }

    printf("not find addr\n");
    fclose(fp);

    return 0;
}

int main(int argc,char *argv[])
{
    pid_t pid = fork();

    if (pid == 0) {
        execl("./code","code",NULL);
    } else if (pid < 0) {
        perror("fork err");
        return -1;
    }

    sleep(1);
    uint64_t base = get_pid_base(pid);
    printf("base = 0x%lx\n",base);

    return 0;
}