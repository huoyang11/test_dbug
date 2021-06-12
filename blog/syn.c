#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <elf.h>

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

int main(int argc,char *argv[])
{
	if (argc < 2) return 0;

	char *word = open_file(argv[1]);

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
    get_all_func(syns,strtab_addr,funs,0);

	struct func_org *p = funs->data;
	for (int i = 0;i < funs->size;i++) {
		printf("%-30s  0x%lx\n",p->name,p->addr);
		p++;
	}

	close_file(word);

	return 0;
}