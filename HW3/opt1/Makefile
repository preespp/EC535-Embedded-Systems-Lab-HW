# [prees26 starlion] Added aggressive optimization flags to boost performance.
CFLAGS = -O3 -funroll-loops

all: qsort_small qsort_large

qsort_small: qsort_small.c Makefile
	gcc $(CFLAGS) qsort_small.c -o qsort_small -lm
qsort_large: qsort_large.c Makefile
	gcc $(CFLAGS) qsort_large.c -o qsort_large -lm

clean:
	rm -rf qsort_small qsort_large output*
