#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/reg.h>
#include <sys/user.h>
#include <sys/syscall.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <elf.h>

#define INT_3 0xcc
#define BUFSIZE 1024
#define str2uint64(str,num) sscanf(str,"%lx",&num)
#define WORD    (sizeof(void *))

struct func_org
{
    char *name;
    uint64_t addr;
};

struct fun_arr
{
    char *word;
    int size;
    struct func_org data[0];
};

int read_pro_mem(pid_t child,uint64_t addr,char *str,int len)
{
    int i = 0;
    int n = len / WORD;
    char *current_p = str;
    uint64_t data;
    //能被整除的部分
    for (;i < n;i++) {
        if ((data = ptrace(PTRACE_PEEKTEXT,child,addr+i*WORD,NULL)) < 0) {
            perror("read mem err");
            return -1;
        }

        memcpy(current_p,&data,WORD);
        current_p += WORD;
    }
    //余数的部分
    int remainder = len % WORD;
    if (remainder != 0) {
        if ((data = ptrace(PTRACE_PEEKTEXT,child,addr+i*WORD,NULL)) < 0) {
            perror("read mem err");
            return -1;
        }
        //拷贝剩余字节数
        memcpy(current_p,&data,remainder);
    }

    return 0;
}

int write_pro_mem(pid_t child,uint64_t addr,char *str,int len)
{
    int i = 0;
    int n = len / WORD;
    char *current_p = str;
    uint64_t data;
    //能被整除的部分
    for (;i < n;i++) {
        memcpy(&data,current_p,WORD);
        if (ptrace(PTRACE_POKETEXT,child,addr +i*WORD,data) < 0) {
            perror("write mem err");
            return -1;
        }
        current_p += WORD;
    }
    //余数的部分
    int remainder = len % WORD;
    if (remainder != 0) {
        //先读整个内存段
        if ((data = ptrace(PTRACE_PEEKTEXT,child,addr+i*WORD,NULL)) < 0) {
            perror("read mem err");
            return -1;
        }
        //把需要改的部分填充掉
        memcpy(&data,current_p,remainder);
        if (ptrace(PTRACE_POKETEXT,child,addr+i*WORD,data) < 0) {
            perror("write mem err");
            return -1;
        }
    }

    return 0;
}

//获取文件大小
int get_file_size(FILE *fp)
{
    if(fp == NULL) return -1;

    fseek(fp, 0, SEEK_END);
    int length = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    return length;
}
//获取文件内容
int get_file_word(FILE *fp,char *word)
{
    if(fp == NULL || word == NULL) return -1;

    int seek = 0;
    while (!feof(fp)) {
        seek += fread(word+seek,1,1024,fp);
    }

    return 0;
}

char *open_file(const char *filepath)
{
    FILE *fp = fopen(filepath,"r");
    if(fp == NULL){
        perror("fopen error");
        return NULL;
    }
    int filesize = get_file_size(fp);
    char *const word = (char *)calloc(1,filesize+1);	
    get_file_word(fp,word);
    fclose(fp);

    return word;
}

void close_file(char *p)
{
    free(p);
}

void *get_section_addr(Elf64_Shdr *stables,char *word,int table_max,int shstrndx,char *sectionname,int *sectionsize)
{
    if(stables == NULL || word == NULL || sectionsize == NULL)  return NULL;
    //字符串表
    char *shstrtab_addr = word + stables[shstrndx].sh_offset;
    //查找段表名
    int i = 0;
    for(i = 0;i < table_max;i++){
        if(strcmp(sectionname,shstrtab_addr + stables[i].sh_name) == 0) break;
    }

    if( i != table_max){
        *sectionsize = stables[i].sh_size;
        return word + stables[i].sh_offset;
    }

    return NULL;
}

int get_all_func(Elf64_Sym *syns,char *strtab_addr,struct fun_arr *funs,uint64_t base)
{
    int count = 0;
    //遍历符号表，找到函数符号
    for(int i = 0;i < funs->size;i++) {
        if ((syns[i].st_info & 0xf) == STT_FUNC && syns[i].st_value != 0) {
            funs->data[count].name = strtab_addr + syns[i].st_name;
            funs->data[count].addr = syns[i].st_value + base;
            count++;
        }
    }
    funs->size = count;

    return 0;
}

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

struct fun_arr *dbuf(const char *path,uint64_t base)
{
    char *word = open_file(path);

    Elf64_Ehdr *ehdr = (Elf64_Ehdr *)word;
    int table_max = ehdr->e_shnum;
    int shstrndx = ehdr->e_shstrndx;
    Elf64_Shdr *stables = (Elf64_Shdr *)(word + ehdr->e_shoff);

    char *shstrtab_addr = word + stables[shstrndx].sh_offset;

    int size = 0;
    //获取符号表的位置
    Elf64_Sym *syns = (Elf64_Sym *)get_section_addr(stables,word,table_max,shstrndx,".symtab",&size);

    int strsize = 0;
    //回去符号名称的位置
    char *strtab_addr = get_section_addr(stables,word,table_max,shstrndx,".strtab",&strsize);
    //Elf64_Sym 数组的大小
    size = size/sizeof(Elf64_Sym);

    struct fun_arr *funs = (struct fun_arr *)calloc(1,sizeof(struct fun_arr) + sizeof(struct func_org) * size);
    funs->size = size;
    funs->word = word;
    //获取函数符号和地址
    get_all_func(syns,strtab_addr,funs,base);

    return funs;
}

void close_dbug(struct fun_arr *p)
{
    close_file(p->word);
    free(p);
}

int find_func(struct fun_arr *funs,const char *func)
{
    if (!funs || !func) return -1;

    for (int i = 0;i < funs->size;i++) {
        if (memcmp(funs->data[i].name,func,strlen(func)) == 0) {
            return i;
        }
    }

    return -1;
}

int main(int argc,char *argv[])
{
    pid_t pid;
    int orig_rax;
    int iscalling = 0;
    int status = 0;
    //一组寄存器的值
    struct user_regs_struct regs;

    if (argc < 2) return 0;

    pid = atoi(argv[1]);

    if (ptrace(PTRACE_ATTACH,pid,NULL,NULL) < 0) {
        perror("attch err");
    }
    //监听子进程的状态
    waitpid(pid,&status,0);
    if (WIFEXITED(status))
        return 0;

    //找到print_int的地址
    uint64_t base = get_pid_base(pid);
    struct fun_arr *funs = dbuf("./dbuf_break_test",base);
    int index = find_func(funs,"print_int");
    uint64_t fun_addr = funs->data[index].addr;
    //往print_int函数插入断点
    uint8_t orig_code = 0;
    read_pro_mem(pid,fun_addr,(void *)&orig_code,1);
    char bit = INT_3;
    write_pro_mem(pid,fun_addr,&bit,1);

    if (ptrace(PTRACE_CONT,pid,NULL,NULL) < 0) {
        perror("CONT err");
        return -1;
    }

    while (1) {
        wait(&status);
        if (WIFEXITED(status))
            break;

        ptrace(PTRACE_GETREGS,pid,NULL,&regs);//获取寄存器整个结构
        
        printf("0x%llx\n",regs.rip);

        //恢复之前的代码值
        write_pro_mem(pid,fun_addr,&orig_code,1);
        //0xcc占一个字节,让ip寄存器恢复
        regs.rip = regs.rip - 1;
        //设置寄存器的值(主要用于恢复ip寄存器)
        ptrace(PTRACE_SETREGS,pid,0,&regs);

        //用于看效果,确保子进程是真的断下
        sleep(1);

        //执行一条汇编指令,然后子进程会暂停
        ptrace(PTRACE_SINGLESTEP,pid,0,0);
        //等待子进程停止
        wait(NULL);

        //断点恢复
        char bit = INT_3;
        write_pro_mem(pid,fun_addr,&bit,1);

        //让子进程在调用系统调用时暂停
        if (ptrace(PTRACE_CONT,pid,NULL,NULL) < 0) {
            perror("CONT err");
            return -1;
        }
    }

    close_dbug(funs);

    return 0;
}