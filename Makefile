CC= g++

COMMON_CPP= *.cpp
COMMON_O= *.o

all: test

test:
	$(CC) -g $(COMMON_CPP) -c
	$(CC) -g $(COMMON_O) -o test

clean:
	rm -f *.o *.exe test
