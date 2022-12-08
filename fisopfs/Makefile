CC = gcc
CFLAGS := -ggdb3 -O2 -Wall -std=c11
CFLAGS += -Wno-unused-function -Wvla

# Flags for FUSE
LDLIBS := $(shell pkg-config fuse --cflags --libs)

SRC = $(wildcard *.c)

OBJS=$(SRCS:.c=.o)

# Name for the filesystem!
FS_NAME := fisopfs

all: fs 

fs: $(SRC)
	$(CC) -g -o $(FS_NAME) $^ $(CFLAGS) $(LDLIBS)
	
build: $(FS_NAME)

format: .clang-files .clang-format
	xargs -r clang-format -i <$<

clean:
	rm -rf $(EXEC) *.o core vgcore.* $(FS_NAME)

.PHONY: all build clean format