main: main.o threadpool.o
	gcc -o main main.o threadpool.o -lpthread

main.o: main.c threadpool.h
	gcc -c main.c

threadpool.o: threadpool.c
	gcc -c threadpool.c

.PHONY:clean
clean:
	rm -rf *.o
	rm -rf main
	



