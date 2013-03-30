shell:shell.o
	gcc shell.o -o shell
shell.o:shell.c
	gcc -c shell.c
clean:
	rm *.o shell
.PHONY:clean
