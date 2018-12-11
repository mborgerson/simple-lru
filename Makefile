lru: main.c lru.c lru.h
	gcc -o $@ -Wall -Werror $(filter %.c, $^)
