#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#define DEVICE "/dev/mytimer"

int main(int argc, char *argv[]) {
    int fd;
    char buffer[256];

    // if number of argument not correct
    if (argc < 2) {
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
       snprintf(buffer, sizeof(buffer), "l");
	
	// Write to device driver
        if (write(fd, buffer, strlen(buffer)) < 0) {
	    close(fd);
	    return EXIT_FAILURE;
        }

       ssize_t len = read(fd, buffer, sizeof(buffer) - 1);
       // In case there is no timer to print
       if (len == 0) {
	   close(fd);
           return EXIT_SUCCESS; 
       } else if (len < 0) {
	   close(fd);
           return EXIT_FAILURE; 
       }
       buffer[len] = '\0';
       printf("%s", buffer);

	// -s is option to write mode
    } else if (strcmp(argv[1], "-s") == 0 && argc == 4) {
      // Retriving input from argument input from command
        int sec = atoi(argv[2]);
	char message[128];
	strncpy(message, argv[3], sizeof(message)-1);
	message[sizeof(message)-1] = '\0';

        snprintf(buffer, sizeof(buffer), "%d %s", sec, message);
	
	// Write to device driver
        if (write(fd, buffer, strlen(buffer)) < 0) {
	  // For printing response message for timer exist
           ssize_t bytes_read = read(fd, buffer, sizeof(buffer) - 1);
	    if (strncmp(buffer, "P", 1) == 0) { // Check if the first character is 'P'
                printf("%s", buffer + 2); // Print from the third character onward
          }
	    close(fd);
	    return EXIT_FAILURE;
        }

	// Read Response from device driver to print message (Timer already exist or timer updated)
	// For printing update message
	{
           ssize_t bytes_read = read(fd, buffer, sizeof(buffer) - 1);
	   if (strncmp(buffer, "P", 1) == 0) { // Check if the first character is 'P'
                printf("%s", buffer + 2); // Print from the third character onward
          }
        }


	// -m is also option to write 
    } else if (strcmp(argv[1], "-m") == 0 && argc == 3) {
        int count = atoi(argv[2]);
        if (count < 1 || count > 5) {
            close(fd);
            return EXIT_FAILURE;
        }
	// To make it different from -s write mode, I send MAX instead in the first argument I sent
        snprintf(buffer, sizeof(buffer), "MAX %d", count);
	// Write to device driver
        if (write(fd, buffer, strlen(buffer)) < 0) {
	  close(fd);
	  return EXIT_FAILURE;
        }
    } else {
        close(fd);
        return EXIT_FAILURE;
    }

    close(fd);
    return EXIT_SUCCESS;
}
