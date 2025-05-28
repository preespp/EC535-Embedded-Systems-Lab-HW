# Cross-compilation and kernel compatibility settings
# This part is for cross-compile for BeagleBone
# prees26 starlion: introduced CROSS_COMPILE to allow building for ARM
CROSS_COMPILE ?= arm-linux-gnueabihf-

# prees26 starlion: use $(CC) instead of gcc directly
CC := $(CROSS_COMPILE)gcc

# prees26 starlion: added Wall, Wextra, debug, profiling, and optimizations
CFLAGS := -Wall -Wextra -O3 -funroll-loops

# Kernel headers (usually not needed for qsort, but kept for consistency)
# prees26 starlion: added path to kernel headers
KERNEL_HEADERS := $(WORKSPACE)/stock-linux-4.19.82-ti-rt-r33
CFLAGS += -I$(KERNEL_HEADERS)/include

# prees26 starlion: link statically when cross-compiling to avoid library issues on BeagleBone
ifeq ($(CROSS_COMPILE),arm-linux-gnueabihf-)
	CFLAGS += -static
endif

# prees26 starlion: define both targets in one place
TARGETS := qsort_large qsort_small

# prees26 starlion: single "all" rule to build both executables
all: $(TARGETS)

# prees26 starlion: changed from "gcc" to "$(CC)" and used $(CFLAGS), plus -lm
qsort_large: qsort_large.c
	$(CC) $(CFLAGS) -o $@ $< -lm

# prees26 starlion: changed from "gcc" to "$(CC)" and used $(CFLAGS), plus -lm
qsort_small: qsort_small.c
	$(CC) $(CFLAGS) -o $@ $< -lm

clean:
# prees26 starlion: remove all compiled output, including gprof data
	rm -f $(TARGETS) gmon.out output*.txt
