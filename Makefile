all:	log_fileSystem_pthread.c log_fileSystem_pthread.h
	gcc -o P1 log_fileSystem_pthread.c -pthread -lm
debug:	log_fileSystem_pthread.c log_fileSystem_pthread.h
	gcc -g -o P1 log_fileSystem_pthread.c -pthread -lm
clean:
	rm -f *.o *~ P1 core
