CC= clang

CFLAGS= -I/opt/homebrew/opt/fmt/include
LFLAGS= -L/opt/homebrew/opt/fmt/lib -lfmt

COMMON_CPP= *.cpp
COMMON_O= *.o

all: test

test:
	$(CC) -g $(COMMON_CPP) -c
	$(CC) -g $(COMMON_O) -o test

test_mac:
	g++-14 -g $(CFLAGS) $(COMMON_CPP) -c
	g++-14 -g $(CFLAGS) $(LFLAGS) $(COMMON_O) -o test


clean:
	rm -f *.o *.exe test
