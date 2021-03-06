CFLAGS=-Wall -c
CC=gcc
CPP=g++

libthreadpool.a: threadpool.o worker.o queue.o counting.o utilities.o
	ar rcs libthreadpool.a threadpool.o worker.o queue.o counting.o utilities.o

test: libthreadpool.a main.c
	$(CC) main.c -L. -lthreadpool -o test

threadpool.o: worker.o queue.o counting.o utilities.o threadpool.c
	$(CC) $(CFLAGS) threadpool.c worker.o queue.o counting.o utilities.o

worker.o: queue.o counting.o utilities.o worker.c
	$(CC) $(CFLAGS) worker.c queue.o counting.o utilities.o

queue.o: counting.o utilities.o queue.c
	$(CC) $(CFLAGS) queue.c counting.o utilities.o

counting.o: utilities.o counting.c
	$(CC) $(CFLAGS) counting.c utilities.o

utilities.o: utilities.c
	$(CC) $(CFLAGS) utilities.c

clean:
	rm *.o *.a test

