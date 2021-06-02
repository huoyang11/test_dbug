CFLARG=-g

main:main.c elf_parse.c dbug.c command.c mem.c ngx/ngx_palloc.c ngx/ngx_array.c
	gcc $(CFLARG) -o $@ $^

test:test.c
	gcc $(CFLARG) -o $@ $^

clean:
	rm -f test main core