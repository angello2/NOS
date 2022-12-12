/* simple program that uses ioctl to send a command to given file */

#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
	int fd_in, fd_out, fd_control, count;
	unsigned long cmd;
	char bufin[64], bufout[64];

	if (argc < 4) {
		fprintf(stderr, "Usage: %s file-in file-out file-control\n", argv[0]);
		return -1;
	}
	
	fd_in = open(argv[1], O_WRONLY);
	if (fd_in == -1) {
		perror("open input failed");
		return -1;
	}	

	fd_control = open(argv[3], O_RDONLY);
	if (fd_control == -1) {
		perror("open control failed");
		return -1;
	}
	
	fd_out = open(argv[2], O_RDONLY);
	if (fd_out == -1) {
		perror("open out failed");
		return -1;
	}
	int option;
	
	while(1) {
		printf("Unesite 0 za izlaz, 1 za write, 2 za read, 3 za ioctl\n");
		printf("\n");
		scanf("%d", &option);
		
		while (option < 0 || option > 3) {
			printf("Opcije su 0, 1, 2 ili 3!\n");
			scanf("%d", &option);
		}
			
		switch(option) {
			case 1: 
				printf("Unesite string (do 64 znaka) koji ce se zapisati u shofer_in\n");
				scanf("%s", bufin);
				size_t len = strlen(bufin);
				if (bufin[len - 1] == '\n') {
					bufin[len - 1] = '\0';
				}
				printf("Unesen string: %s\n", bufin);
				write(fd_in, bufin, sizeof(bufin));
				printf("String zapisan na shofer_in\n");
				printf("\n");
				break;
			case 2:
				printf("Citam sa shofer_out...\n");
				read(fd_out, bufout, sizeof(bufout));
				printf("Procitano: %s\n", bufout);
				printf("\n");
				break;
			case 3: 
				printf("Pozvan ioctl. Unesite broj bajtova koji ce se prepisati iz shofer_in u shofer_out:"); 
				scanf("%ld", &cmd);
				count = ioctl(fd_control, cmd);
				printf("ioctl vraca %d\n", count);
				printf("\n");
				break;	
			case 0:
				return 0;	
		}
	}	
	return 0;
}
