// Pree Simphliphan
// U01702082

#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <stdio.h>

#define DEVICE "/dev/mytimer"
#define PROC_TIMER "/proc/mytimer"

void sighandler(int);

// Helper function to past timer
void int_to_str(int num, char *str) {
    int i = 0;
    if (num == 0) {
        str[i++] = '0';
    } else {
        int temp = num;
        while (temp > 0) {
            str[i++] = (temp % 10) + '0';
            temp /= 10;
        }
    }
    str[i] = '\0';

    // Reverse the string (because we've built it backwards)
    int start = 0;
    int end = i - 1;
    while (start < end) {
        char tmp = str[start];
        str[start] = str[end];
        str[end] = tmp;
        start++;
        end--;
    }
}

int main(int argc, char *argv[]) {
    int fd;
    char buffer[256];
    int  oflags;
    struct sigaction action;
    char num_str[10];  // This will hold the string representation of the number

    // if number of argument not correct
    if (argc < 2) {
	write(STDOUT_FILENO, "Argument number incorrect\n", 26);
        return EXIT_FAILURE;
    }

    fd = open(DEVICE, O_RDWR);
    // if fail to open device
    if (fd < 0) {
        close(fd);
        return EXIT_FAILURE;
    }

    // -l is option to list active timers (read)
    if (strcmp(argv[1], "-l") == 0) {
      // to change mode in ktimer to l
       buffer[0] = 'l';
       buffer[1] = '\0';
       // snprintf(buffer, sizeof(buffer), "l");
	
	// Write to device driver
        if (write(fd, buffer, strlen(buffer)) < 0) {
	    close(fd);
	    return EXIT_FAILURE;
        }

        ssize_t bytes_read = read(fd, buffer, sizeof(buffer) - 1);
	//buffer[bytes_read] = '\0';
        write(STDOUT_FILENO, buffer, bytes_read);
        close(fd);

	// -s is option to write mode
    } else if (strcmp(argv[1], "-s") == 0 && argc == 4) {
      // Retriving input from argument input from command
        int sec = atoi(argv[2]);
	char message[128];
	strncpy(message, argv[3], sizeof(message)-1);
	message[sizeof(message)-1] = '\0';


	int_to_str(sec, num_str);

        // Copy the number string into the buffer
	int buffer_index = 0;
	for (int j = 0; num_str[j] != '\0'; j++) {
	  buffer[buffer_index++] = num_str[j];  // Copy each digit of the number
	}

	//  Add a space after the integer
	buffer[buffer_index++] = ' ';

	 // Copy the message string into the buffer
	for (int k = 0; message[k] != '\0'; k++) {
	  buffer[buffer_index++] = message[k];  // Copy each character of the message
	}

        //  Null-terminate the final combined string
	buffer[buffer_index] = '\0';

	// snprintf(buffer, sizeof(buffer), "%d %s", sec, message);
	
	// Write to device driver
        if (write(fd, buffer, strlen(buffer)) < 0) {
	  // For printing response message for timer exist
           ssize_t bytes_read = read(fd, buffer, sizeof(buffer) - 1);
	   //buffer[bytes_read] = '\0';
	    if (strncmp(buffer, "P", 1) == 0) { // Check if the first character is 'P'
	        write(STDOUT_FILENO, buffer + 2, strlen(buffer) - 2); // Print from the third character onward
          }
	    close(fd);
	    return EXIT_FAILURE;
        }

	// Read Response from device driver to print message (Timer already exist or timer updated)
	// For printing update message
	{
           ssize_t bytes_read = read(fd, buffer, sizeof(buffer) - 1);
	   //buffer[bytes_read] = '\0';
	   if (strncmp(buffer, "P", 1) == 0) { // Check if the first character is 'P'
		write(STDOUT_FILENO, buffer + 2, strlen(buffer) - 2); // Print from the third character onward
          }
        }


	// -m is also option to write 
    } else if (strcmp(argv[1], "-m") == 0 && argc == 3) {
        int count = atoi(argv[2]);
	char message[128];
	// To make it different from -s write mode, I send MAX instead in the first argument I sent
        //snprintf(buffer, sizeof(buffer), "MAX %d", count);

	int_to_str(count, num_str);

        // Copy the number string into the buffer
	int buffer_index = 0;
	strncpy(message, "MAX", sizeof(message)-1);
	// Copy the message string into the buffer
	for (int k = 0; message[k] != '\0'; k++) {
	  buffer[buffer_index++] = message[k];  // Copy each character of the message
	}

	//  Add a space after the integer
	buffer[buffer_index++] = ' ';

	for (int j = 0; num_str[j] != '\0'; j++) {
	  buffer[buffer_index++] = num_str[j];  // Copy each digit of the number
	}

        //  Null-terminate the final combined string
	buffer[buffer_index] = '\0';

	// Write to device driver
        if (write(fd, buffer, strlen(buffer)) < 0) {
	  close(fd);
	  return EXIT_FAILURE;
        }

	 // Print Error message from -m
	 ssize_t bytes_read = read(fd, buffer, sizeof(buffer) - 1);
	 //buffer[bytes_read] = '\0';
	   if (strncmp(buffer, "P", 1) == 0) { // Check if the first character is 'P'
	       write(STDOUT_FILENO, buffer + 2, strlen(buffer) - 2); // Print from the third character onward
          }

	// -r option to remove timer
	
    } else if (strcmp(argv[1], "-r") == 0) {
        strncpy(buffer, "REMOVE", sizeof(buffer)-1);
        //snprintf(buffer, sizeof(buffer), "REMOVE");
	// Write to device driver
        if (write(fd, buffer, strlen(buffer)) < 0) {
	  close(fd);
	  return EXIT_FAILURE;
        }
    } else {
        close(fd);
        return EXIT_FAILURE;
    }

    // Setup SIGIO signal handler
    memset(&action, 0, sizeof(action));
    action.sa_handler = sighandler;
    action.sa_flags = SA_SIGINFO;
    sigemptyset(&action.sa_mask);
    sigaction(SIGIO, &action, NULL);
    fcntl(fd, F_SETOWN, getpid());
    oflags = fcntl(fd, F_GETFL);
    fcntl(fd, F_SETFL, oflags | FASYNC);

    // Now wait for the signal
    pause();

    close(fd);
    return EXIT_SUCCESS;
}

// SIGIO handler
void sighandler(int signo){
  int fd = open(DEVICE, O_RDWR);
    char buffer[256];
    //if fail to open device
   if (fd < 0) {
    printf("Fail\n");
        close(fd);
    return EXIT_FAILURE;
      }
   ssize_t bytes_read = read(fd, buffer, sizeof(buffer) - 1);
 write(STDOUT_FILENO, buffer, strlen(buffer)); // Print from the third character onward

 close(fd);
}
