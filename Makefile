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
