# Compiler
CC = gcc

# Compiler flags
CFLAGS = -Wall -pthread

# Source files
SRC = main.c

# Object files
OBJ = $(SRC:.c=.o)

# Executable name
EXEC = cpu_monitor

.PHONY: all clean

all: $(EXEC)

$(EXEC): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f $(OBJ) $(EXEC)

##########################################
## Compiler and compiler flags
#CC = gcc
#CFLAGS = -Wall -Wextra -Werror -std=c99
#LDFLAGS =

## Source files
#SRC = cpu_monitor.c main.c
#TEST_SRC = cpu_monitor.c main_test.c

# Object files
#OBJ = $(SRC:.c=.o)
#TEST_OBJ = $(TEST_SRC:.c=.o)

## Executable
#EXEC = cpu_monitor
#TEST_EXEC = test_cpu_monitor

## Targets
#.PHONY: all clean test

#all: $(EXEC)

#$(EXEC): $(OBJ)
#	$(CC) $(LDFLAGS) $^ -o $@

#test: $(TEST_EXEC)
#	./$(TEST_EXEC)

#$(TEST_EXEC): $(TEST_OBJ)
#	$(CC) $(LDFLAGS) $^ -o $@

#%.o: %.c
#	$(CC) $(CFLAGS) -c $< -o $@

#clean:
#	rm -f $(OBJ) $(EXEC) $(TEST_OBJ) $(TEST_EXEC)
