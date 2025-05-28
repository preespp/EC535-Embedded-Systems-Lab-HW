# Pree Simphliphan
# U01702082

# Compiler and Compiler flags
CC = gcc           
CFLAGS = -Wall -O3 -march=native -flto -ftree-vectorize -funroll-loops

# Object files and target executable
OBJS = simpleISS.o    
TARGET = simpleISS          

# Default rule
all: $(TARGET)

# Rule to link object files and create the executable
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -MMD -o $(TARGET) $(OBJS)

# Rule to clean up object files and executable
clean:
	rm -f  $(OBJS) $(TARGET)