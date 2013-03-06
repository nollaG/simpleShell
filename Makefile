shell:shell.o
	clang -ggdb shell.o -o shell
shell.o:shell.c
	clang -ggdb -c shell.c
clean:
	rm *.o shell
.PHONY:clean
