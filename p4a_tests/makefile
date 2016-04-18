C_FILES := $(wildcard *.c)
TARGETS := ${C_FILES:.c=}

all: ${TARGETS}

%: %.c
	gcc -g -L.. -lcrawler -Xlinker -rpath=. -lpthread ../cs537.c -Wall -Werror -o $@ $<

clean:
	rm -rf ${TARGETS} *.o

