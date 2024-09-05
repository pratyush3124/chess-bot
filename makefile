CC ?= gcc 
SRCS = $(wildcard src/*.c)

EXE ?= MGBOT

WARN_FLAGS = -Wall -Werror -Wextra -Wno-error=vla -Wpedantic -Wno-unused-command-line-argument
OPT_FLAGS = -Ofast -march=native -funroll-loops

MISC_FLAGS ?=

ifeq ($(CC), clang)
	OPT_FLAGS = -O3 -ffast-math -flto -march=native -funroll-loops
endif

LIBS = -lm

all:
	$(CC) $(SRCS) -o $(EXE) $(OPT_FLAGS) $(MISC_FLAGS) $(LIBS)

clean:
	rm -f $(EXE)