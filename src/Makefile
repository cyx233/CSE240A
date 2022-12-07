CC=gcc
OPTS=-g -std=c99 -Werror

all: main.o predictor.o
	$(CC) $(OPTS) -o predictor main.o predictor.o -lm
	$(CC) $(OPTS) -o predictor_NN main.o NN.o -lm

main.o: main.c predictor.h
	$(CC) $(OPTS) -c main.c

predictor.o: predictor.h predictor.c
	$(CC) $(OPTS) -c predictor.c
	$(CC) $(OPTS) -c NN.c

clean:
	rm -f *.o predictor;
