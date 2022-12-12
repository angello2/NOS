#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
						
int main (int argc, char *argv[])
{	
	if (argc < 2) {
		fprintf(stderr, "Usage: %s file...\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	
	srand(time(NULL));
	
	// Create a list of pollfd structs for the given file descriptors
	int num_fds = argc - 1;
	struct pollfd pfds[num_fds];
	for (int i = 0; i < num_fds; i++) {
		pfds[i].fd = open(argv[i + 1], O_WRONLY);
		pfds[i].events = POLLOUT;
		printf("Opened %s on fd%d\n", argv[i + 1], pfds[i].fd);
	}
		
	// Poll the file descriptors every 5 seconds to check for writability, up to 20 times	
	int count = 20;
	while (count > 0) {
		int ready = poll(pfds, num_fds, -1);
		if (ready > 0) {
			// if one or more fds are ready for writing, choose a random one and write to it
			int ready_fds = ready;
			int rand_fd = rand() % ready_fds;
			char c = 'a';
			printf("Writing 'a' to fd%d (%s)\n", pfds[rand_fd].fd, argv[rand_fd + 1]);
			write(pfds[rand_fd].fd, &c, 1);
		}
		count--;
		sleep(5);
	}
	return 0;	
}
