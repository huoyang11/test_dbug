#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <elf.h>

#include "elf_parse.h"

#define BUFSIZE 1024
#define str2uint64(str,num) sscanf(str,"%lx",&num)
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
//获取程序基地址
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

int get_all_func(Elf64_Sym *syns,char *strtab_addr,struct fun_arr *funs,uint64_t base)
{
    int count = 0;

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

void *get_section_addr(Elf64_Shdr *stables,char *word,int table_max,int shstrndx,char *sectionname,int *sectionsize)
{
    if(stables == NULL || word == NULL || sectionsize == NULL)  return NULL;

    char *shstrtab_addr = word + stables[shstrndx].sh_offset;

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

struct fun_arr *open_funs(pid_t pid)
{
    uint64_t base = get_pid_base(pid);

    char buf[1024] = {0};
    char *pro_exe_path = buf;
    pro_exe_path += sprintf(pro_exe_path,"%s","/proc");
    pro_exe_path += sprintf(pro_exe_path,"/%d",pid);
    sprintf(pro_exe_path,"/%s","exe");
    char target[100] = {0};
    int target_len = readlink(buf,target,100);
    target[target_len] = 0;

    FILE *fp = fopen(target,"r");
    if(fp == NULL){
        perror("fopen error");
        return NULL;
    }
    int filesize = get_file_size(fp);
    char *const word = (char *)calloc(1,filesize+1);	
    get_file_word(fp,word);
    fclose(fp);

    Elf64_Ehdr *ehdr = (Elf64_Ehdr *)word;//elf文件头
    int table_max = ehdr->e_shnum;//段表的数量
    int shstrndx = ehdr->e_shstrndx;//字符串表的下标
    Elf64_Shdr *stables = (Elf64_Shdr *)(word + ehdr->e_shoff);//段表

    //获取符号表
    int size = 0;
    Elf64_Sym *syns = (Elf64_Sym *)get_section_addr(stables,word,table_max,shstrndx,".symtab",&size);

    //获取符号表字符串
    int strsize = 0;
    char *strtab_addr = get_section_addr(stables,word,table_max,shstrndx,".strtab",&strsize);

    size = size/sizeof(Elf64_Sym);
    struct fun_arr *funs = (struct fun_arr *)calloc(1,sizeof(struct fun_arr) + sizeof(struct func_org) * size);
    funs->size = size;
    funs->word = word;
    get_all_func(syns,strtab_addr,funs,base);

    return funs;
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

void foreach_fun(struct fun_arr *funs)
{
    if (!funs) return;

    for (int i = 0;i < funs->size;i++) {
        printf("%-3d func:%-25s addr:%lx\n",i,funs->data[i].name,funs->data[i].addr);
    }
}

int close_funs(struct fun_arr *funs)
{
    if (!funs) return -1;
    char *word = funs->word;

    free(funs);
    free(word);

    return 0;
}