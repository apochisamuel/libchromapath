CC ?= clang
CXX ?= clang++
AR ?= ar
CFLAGS ?= -std=c11 -O2 -Wall -Wextra -Wpedantic -Isrc
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -Wpedantic -Isrc
SRC := $(wildcard src/*.c)
OBJ := $(SRC:.c=.o)

all: libchromapath.a chromapath-inspect test-basic

libchromapath.a: $(OBJ)
	$(AR) rcs $@ $^

chromapath-inspect: tools/inspect.c libchromapath.a
	$(CC) $(CFLAGS) -o $@ tools/inspect.c libchromapath.a -lm

test-basic: tests/test_basic.c libchromapath.a
	$(CC) $(CFLAGS) -o $@ tests/test_basic.c libchromapath.a -lm

test: test-basic
	./test-basic

clean:
	rm -f src/*.o libchromapath.a chromapath-inspect test-basic

.PHONY: all test clean
