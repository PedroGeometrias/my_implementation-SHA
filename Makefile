# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -O2 -g
LDFLAGS = -lm 

# Source and output files
SRC = main.c buffer.c
OBJ = main.o buffer.o
EXE = SHA
ASM = main.s

# Default target: Build the executable
all: $(EXE)

# Compile object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Link the final executable
$(EXE): $(OBJ)
	$(CC) $(CFLAGS) -o $(EXE) $(OBJ) $(LDFLAGS)

# Generate Assembly output
assembly: $(SRC)
	$(CC) $(CFLAGS) -S $(SRC) -o $(ASM)

# Clean up generated files
clean:
	rm -f *.o $(EXE) $(ASM)

.PHONY: all clean assembly

