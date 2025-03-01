# This is useless unless you just do not want to make your own makefile for your project and want to modify this

CC= g++

CFLAGS= -O2
# get rid of pthread if you are on windows
LFLAGS= -lfmt -lpthread

SRCS = src/vtkParser.cpp \
	   src/meshParse.c \
	   src/bridethread.c
OBJS = $(SRCS:.cpp=.o)
OBJS := $(OBJS:.c=.o)


.PHONY: all build clean

all: build

%.o: %.cpp
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

build: $(OBJS)

clean:
	rm -f src/*.o $(TARGET)
