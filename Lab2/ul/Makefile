# Pree Simphliphan
# U01702082

# Compiler settings
CROSS_COMPILE = arm-linux-gnueabihf-
CFLAGS = -Wall -Wextra

# Path to kernel headers
KERNEL_HEADERS = $(WORKSPACE)/stock-linux-4.19.82-ti-rt-r33

# Include kernel headers for compatibility
CFLAGS += -I$(KERNEL_HEADERS)/include

# Static to avoid issue with library
ifeq ($(CROSS_COMPILE),arm-linux-gnueabihf-)
	CFLAGS += -static
endif

CC = $(CROSS_COMPILE)gcc

# Object files and target executable
OBJS = ktimer.c
TARGET = ktimer

# Default rule
all: $(TARGET)

# Rule to compile ktimer
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

# Rule to clean up object and executable files
clean:
	rm -f $(TARGET)
