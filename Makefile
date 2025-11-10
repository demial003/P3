CC = gcc
# CFLAGS = -std=c99 -g -Wall -fsanitize=address,undefined

mysh: mysh.c arraylist.c
	$(CC) -o mysh mysh.c arraylist.c

# arraylist.o: arraylist.c arraylist.h
# 	$(CC) arraylist.c
