shell:shell.o
	gcc -std=c99 shell.o -o shell
shell.o:shell.c
	gcc -std=c99 -c shell.c
clean:
	rm *.o shell
.PHONY:clean
