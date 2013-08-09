CC = gcc
FLAGS = -Wall -std=c99
RM = rm

ft : ft.c
	$(CC) $(FLAGS) -o $@ $<

run :
	./ft

clean :
	$(RM) ft
