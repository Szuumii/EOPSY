NAME=philosophers

COMPLIER= gcc

CFLAGS= -pthread -Wall

compile: philosophers.o
	$(COMPILER) $(CFLAGS) -o $(NAME) philosophers.o

philosophers.o:
	$(COMPILER) -c philosophers.c

run:
	./philosophers

clean:
	rm philosophers.o $(NAME)
