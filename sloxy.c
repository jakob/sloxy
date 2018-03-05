#import <stdlib.h>
#import <stdio.h>
#import <strings.h>
#import <sys/types.h>
#import <sys/uio.h>
#import <sys/socket.h>
#import <sys/un.h>
#import <sys/select.h>
#import <sys/time.h>
#import <netdb.h>
#import <unistd.h>
#import <arpa/inet.h>
#import <math.h>

struct forward_task {
	int incoming;
	int outgoing;
	struct forward_task *next;
};

char *buffer;
int buffer_size;
double speedlimit = 5000;
double delay = 0.1;

int readwrite(int from, int to);

int main(int argc, char **argv) {
	
	if (argc != 7) {
		fprintf(stderr, "Usage: %s listen_addr listen_port destination_addr destination_port speed_limit delay\n", argv[0]);
		exit(1);
	}
	
	char *listen_address = argv[1];
	uint16_t listen_port = atoi(argv[2]);
	char *target_address = argv[3];
	uint16_t target_port = atoi(argv[4]);
	
	speedlimit = atof(argv[5]);
	delay = atof(argv[6]);
	
	buffer_size = speedlimit;
	if (buffer_size > 1000000) buffer_size = 1000000;
	buffer = malloc(buffer_size);	
	
	printf("      Delay: %gms\n", delay * 1000);
	printf("Speed Limit: %gKB/s\n", speedlimit/1000 );
	printf("Buffer Size: %d bytes\n", buffer_size );
	
	printf("\n");
	
	int listen_socket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (!listen_socket) {
		perror("socket()");
		exit(1);
	}
	
	struct sockaddr_in *listen_sockaddr = malloc(sizeof *listen_sockaddr);
	bzero(listen_sockaddr, sizeof *listen_sockaddr);
	listen_sockaddr->sin_family = AF_INET;
	listen_sockaddr->sin_len = sizeof *listen_sockaddr;
	listen_sockaddr->sin_port = htons(listen_port);
	if (INADDR_NONE == (listen_sockaddr->sin_addr.s_addr = inet_addr(listen_address))) {
		fprintf(stderr, "listen_address is not a valid IPv4 address\n");
		exit(1);
	}
	
	int yes = 1;
	setsockopt(listen_socket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
	
	int bind_result = bind(listen_socket, (struct sockaddr *)listen_sockaddr, sizeof(*listen_sockaddr));
	if (bind_result != 0) {
		perror("bind()");
		exit(1);
	}
	free(listen_sockaddr);
	
	int listen_result = listen(listen_socket, 1);
	if (listen_result) {
		perror("listen()");
		exit(1);
	}
	
	
	struct forward_task *fwtask = NULL;
	
	int max_socket = listen_socket;
	
	while (1) {
		fd_set rfds;
		FD_ZERO(&rfds);

		FD_SET(listen_socket, &rfds);
		
		{
			struct forward_task *t = fwtask;
			while (t) {
				FD_SET(t->incoming, &rfds);
				FD_SET(t->outgoing, &rfds);
				t = t->next;
			}
		}
		
		if (!fwtask) {
			printf("Listening on %s:%d\n", listen_address, listen_port);
		}
		
		int select_result = select(max_socket+1, &rfds, NULL, NULL, NULL);
		if(select_result < 0)
		{
			perror("select()");
			exit(1);
		}
				
		if (FD_ISSET(listen_socket, &rfds)) {
			printf("Connecting to %s:%d\n", target_address, target_port);
			struct forward_task **lastaddr = &fwtask;
			while (*lastaddr) lastaddr = &((*lastaddr)->next);
			struct forward_task *newTask = *lastaddr = calloc(sizeof(struct forward_task), 1);
			
			newTask->incoming = accept(listen_socket, NULL, NULL);
			if(newTask->incoming <= 0)
			{
				perror("accept()");
				exit(1);
			}
			if (newTask->incoming > max_socket) max_socket = newTask->incoming;

			struct sockaddr_in *target_sockaddr = malloc(sizeof *target_sockaddr);
			bzero(target_sockaddr, sizeof *target_sockaddr);
			target_sockaddr->sin_family = AF_INET;
			target_sockaddr->sin_len = sizeof *target_sockaddr;
			target_sockaddr->sin_port = htons(target_port);
			if (INADDR_NONE == (target_sockaddr->sin_addr.s_addr = inet_addr(target_address))) {
				fprintf(stderr, "destination_addr is not a valid IPv4 address\n");
				exit(1);
			}
			
			newTask->outgoing = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
			if (!newTask->outgoing) {
				perror("socket()");
				exit(1);
			}
			if (newTask->outgoing > max_socket) max_socket = newTask->outgoing;
			
			int connect_result = connect(newTask->outgoing, (struct sockaddr *)target_sockaddr, sizeof(*target_sockaddr));
			if (connect_result) {
				perror("connect()");
				exit(1);
			}
			
			printf("Connected.\n");
		}
		
		{
			struct forward_task **taddr = &fwtask;
			while (*taddr) {
				struct forward_task *t = *taddr;
				int shouldClose = 0;
				if (FD_ISSET(t->incoming, &rfds)) {
					shouldClose = readwrite(t->incoming, t->outgoing);
				}
				if (!shouldClose && FD_ISSET(t->outgoing, &rfds)) {
					shouldClose = readwrite(t->outgoing, t->incoming);
					
				}
				if (shouldClose) {
					close(t->incoming);
					close(t->outgoing);
					*taddr = (t->next);
					free(t);
					printf("Disconnected.\n");
				} else {
					taddr = &(t->next);
				}
			}
		}
	}
}

int readwrite(int from, int to) {
	int readbytes = read(from, buffer, buffer_size);
	if (readbytes==0) {
		return 1;
	}
	if (readbytes<0) {
		perror("read()");
		exit(1);
	}
	
	double wait_time = readbytes / speedlimit;
	if (readbytes<buffer_size) wait_time += delay;
		
	struct timespec tv = {0};
	tv.tv_sec = floor(wait_time);
	tv.tv_nsec = (wait_time-floor(wait_time)) * 1e9;
	nanosleep(&tv, NULL);
	
	char *writebuffer = buffer;
	while (readbytes) {
		int writtenbytes = write(to, writebuffer, readbytes);
		if (writtenbytes<0) {
			perror("write()");
			exit(1);
		}
		writebuffer += writtenbytes;
		readbytes -= writtenbytes;
	}
	return 0;
}