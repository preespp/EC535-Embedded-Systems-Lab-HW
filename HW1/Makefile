# Pree Simphliphan
# U01702082

# Compiler and Compiler flags
CC = gcc           
CFLAGS = -Wall -Wextra -g 

# Object files and target executable
OBJS = main.o bits.o     
TARGET = BitApp          

# Default rule: builds the target executable
all: $(TARGET)

# Rule to link object files and create the executable
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

# Rule to compile main.c
main.o: main.c bits.h
	$(CC) $(CFLAGS) -c main.c

# Rule to compile bits.c
bits.o: bits.c bits.h
	$(CC) $(CFLAGS) -c bits.c

# Rule to clean up object files and executable
clean:
	rm -f $(OBJS) $(TARGET)
