CC=g++
CFLAGS=-c -Wall -std=c++11
LFLAGS=-lpthread

all: main

main: fizzbuzz.o
	$(CC) fizzbuzz.o -o main $(LFLAGS) 

fizzbuzz.o:
	$(CC) $(CFLAGS) fizzbuzz.cpp

clean:
	rm fizzbuzz.o main
