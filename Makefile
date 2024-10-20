CC= g++

COMMON_CPP= *.cpp
COMMON_O= *.o

all: test

test:
	$(CC) $(COMMON_CPP) -c
	$(CC) $(COMMON_O) -o test
	
