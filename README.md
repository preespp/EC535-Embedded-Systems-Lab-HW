# Embedded Systems Lab & HW

## Lab

Lab 1: Custom Embedded Linux Distribution

- Compiled a custom Linux kernel and created a root filesystem using BusyBox
- Configured kernel modules using menuconfig and cross-compiled the image for deployment via QEMU emulator.
- Learned fundamentals of embedded bootloaders, kernel images (zImage/uImage), ext4 file systems, and Linux startup sequence.
- Built a bootable SD card image using shell scripts and partitioning tools, and validated the image with QEMU emulator.

Lab 2: Linux Kernel Module and Timer System

- Developed and tested custom Linux kernel modules for ARM architecture on a BeagleBone Linux image.
- Implemented a kernel timer that prints user-specified messages after configurable delays.
- Wrote a user-space C program (ktimer) to interact with the kernel module via a character device driver.
- Kernel development practices: Makefiles, module loading (insmod, rmmod), /dev files, and timers.

Lab 3: Asynchronous Event-Driven Kernel Module with /proc Interface

- Extended the timer kernel module to support asynchronous I/O notifications using fasync and SIGIO signals.
- Enabled real-time signaling from kernelspace to userspace when a timer expires.
- Created a custom /proc/mytimer file using the Linux procfs API to expose module state, including PID and command info.
- Improved robustness by synchronizing kernel and user state and handling edge cases in concurrent timer access.

Lab 4: Real-Time GPIO Control on BeagleBone Black

- Developed a kernel-space driver to control a model traffic light system using GPIO pins on real hardware.
- Mapped physical GPIO pins to logical identifiers, set up input/output configurations, and implemented interrupt-based input handling for pushbuttons.
- Worked with low-level GPIO APIs (gpio_to_desc, gpiod_get, gpiod_set_value) inside the kernel.
- Assembled hardware using breadboards, resistors, LEDs, and switches; flashed SD card with custom-built system image; tested system end-to-end using serial and SSH debugging.

## Homework

Homework 1: C program to process a list of 32-bit little-endian integers, converting to big-endian, encrypting them using a 4-letter XOR cipher, and formatting output to include decimal, hexadecimal, and ASCII representation.

- Implemented the solution modularly across main.c, bits.c, and bits.h, and managed compilation dependencies with a custom Makefile.
- Designed and implemented a linked list structure to sort data based on encrypted ASCII values before output.
- Practiced modular programming, endian manipulation, ASCII transformations, and bitwise logic.

Homework 2: Custom 8-bit Instruction Set Simulator in C to emulate a basic 8-bit processor with 6 registers and 256B local memory.

- Supported key operations in Assembly (e.g., MOV, ADD, LD, ST, CMP, JE, JMP) and implemented instruction counting, cycle calculation, cache simulation, and local memory hit tracking.
- Designed a simplified memory hierarchy, where local memory cached external memory accesses with differing cycle penalties.
- Ensured robust parsing and error handling of disassembled assembly input files and implemented execution statistics output.

Homework 3: Performance Optimization on Embedded Benchmark

- Profiled and optimized the qsort_large benchmark from the MiBench embedded suite using gprof on both Linux machines and the BeagleBone Black.
- Performed compiler-level optimization and source-code level optimization.
- Analyzed execution times before and after optimization on both x86 and ARM targets (BeagleBone).
