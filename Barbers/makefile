NAME=barbers

COMPILER=gcc

FLAGS= -W -std=c99

compile: barber.o
	$(COMPILER) $(FLAGS) -o $(NAME) barbers.o

barber.o :
	gcc -c barbers.c

run:
	./barbers

clean:
	rm barbers.o barbers
