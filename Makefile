CURR_DIR=$(notdir $(basename $(shell pwd)))
PRJ=$(CURR_DIR)
SRC=$(filter-out $(wildcard ref*.c), $(wildcard *.c))
OBJ=$(patsubst %.c,%.o,$(SRC))

BENCH_FLAGS?=-O2
BINARY_OUTPUT_PATH?=$@

CC=gcc
INCLUDES=-I../../include
ifndef DEBUG
CFLAGS=-std=gnu99 -fopenmp $(BENCH_FLAGS) 
LIB=
else
CFLAGS=-O0 -g3 -std=gnu99 -fopenmp
LIB=
endif

all: $(PRJ)

$(PRJ): $(OBJ)
	$(CC) $(CFLAGS) $(INCLUDES) $(OBJ) -o $(BINARY_OUTPUT_PATH)

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@ $(LIB)	

clean:
	-rm -f $(OBJ) $(PRJ) 
