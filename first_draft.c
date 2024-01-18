#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <sys/select.h>
#include <sys/types.h> 
#include <stdio.h>
#include <stdlib.h>

int total_clients = 0;
int max_fd = 0;

fd_set readfds, writefds, cpyfds;

typedef struct s_client
{
	int fd;
	int id;
	char *message_buffer;
} t_client;


void erro_msg(char *msg)
{
	write(2, msg, strlen(msg));
	exit(1);
}

void maxi(t_client *clients, int *maxFd, int totalClients) {
    for (int i = 0; i < totalClients; i++) {
        if (clients[i].fd > *maxFd)
            *maxFd = clients[i].fd;
    }
}

void send_all(t_client *clients, int totalClients, char *msg, int senderFd) {
    for (int i = 0; i < totalClients; i++) {
        if (clients[i].fd != senderFd && clients[i].fd != -1)
            send(clients[i].fd, msg, strlen(msg), 0);
    }
}

int extract_message(char **buf, char **msg)
{
	char	*newbuf;
	int	i;

	*msg = 0;
	if (*buf == 0)
		return (0);
	i = 0;
	while ((*buf)[i])
	{
		if ((*buf)[i] == '\n')
		{
			newbuf = calloc(1, sizeof(*newbuf) * (strlen(*buf + i + 1) + 1));
			if (newbuf == 0)
				return (-1);
			strcpy(newbuf, *buf + i + 1);
			*msg = *buf;
			(*msg)[i + 1] = 0;
			*buf = newbuf;
			return (1);
		}
		i++;
	}
	return (0);
}

char *str_join(char *buf, char *add)
{
	char	*newbuf;
	int		len;

	if (buf == 0)
		len = 0;
	else
		len = strlen(buf);
	newbuf = malloc(sizeof(*newbuf) * (len + strlen(add) + 1));
	if (newbuf == 0)
		return (0);
	newbuf[0] = 0;
	if (buf != 0)
		strcat(newbuf, buf);
	free(buf);
	strcat(newbuf, add);
	return (newbuf);
}


int main(int argc, char **argv) {
	if (argc != 2)
		erro_msg("Wrong number of arguments\n");
	printf("Server started. Connecting on port: %d\n", atoi(argv[1]));

	int sockfd, connfd;
	socklen_t len;
	struct sockaddr_in servaddr, cli;

	// socket - create an endpoint for communication and return a file descriptor for the server socket
	sockfd = socket(AF_INET, SOCK_STREAM, 0); 
	if (sockfd == -1)
		erro_msg("Fatal error\n");

	// bzero - write zeroes to a byte string effectively clearing it
	bzero(&servaddr, sizeof(servaddr)); 

	// servaddr.sin_family = AF_INET - set address family to IPv4
	servaddr.sin_family = AF_INET; 
	// servaddr.sin_addr.s_addr = htonl(2130706433) - set address to 127.0.0.1
	servaddr.sin_addr.s_addr = htonl(2130706433);
	// servaddr.sin_port = htons(atoi(argv[1])) - set port to the one passed as an argument (eg. 8081)
	servaddr.sin_port = htons(atoi(argv[1]));

	// bind - bind a name to a socket (sockfd) and check if it was successful. Binding is needed to listen on a port
	if ((bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr))) != 0)
		erro_msg("Fatal error\n");
	
	// listen - listen for connections on a socket (sockfd) and set the backlog queue to 10
	if (listen(sockfd, 10) != 0)
		erro_msg("Fatal error\n");

	printf("Server initialized. Family: %d, Address: %d, Port: %d\n", servaddr.sin_family, servaddr.sin_addr.s_addr, servaddr.sin_port);

	// Initialize fd sets
 	FD_ZERO(&cpyfds);
	// FD_SET - add fd to the set (adding the server fd to the set)
	FD_SET(sockfd, &cpyfds);

	max_fd = sockfd;

	printf("Server file descriptor: %d\n", sockfd);

	// Client structure array - holds all clients. We initalize it with 4096 to prevent memory leaks
	t_client clients[4096];

	printf("Clients structure initialized, size: %lu\n", sizeof(clients));
	printf("Clients structure pointer: %p\n", clients);

	while (1)
	{
		// Set readfds and writefds to cpyfds - This is needed because select modifies the set it's given and we need to keep the original
		readfds = writefds = cpyfds;

		// select - synchronous I/O multiplexing system call. Checks if there is data to read or write
		if (select(max_fd + 1, &readfds, &writefds,0 , 0) < 0) {
		}
		else
		{
			// Cycle through all file descriptors starting from the server fd (sockfd (3)))
			for (int fd = sockfd; fd <= max_fd; fd++)
			{
				if (FD_ISSET(fd, &readfds))
				{
					// If the fd is the server fd, accept the connection
					if (fd == sockfd)
					{
						// len = sizeof(cli) - get size of the client structure
						len = sizeof(cli);
						// accept - accept a connection on a socket and returns a new file descriptor created for the new connection
						connfd = accept(sockfd, (struct sockaddr *)&cli, &len);
						// connfd - new file descriptor returns -1 if error which we will ignore for this exercise
						if (connfd < 0) {
						} 
						else
						{
							// Keep track of total clients connected, so we can cycle through them later
							int id = total_clients++;

							// Fill the client structure with relevant data
							clients[id].fd = connfd;
							clients[id].id = id;
							clients[id].message_buffer = NULL;

							char msg[50] = {'\0'};
							// sprintf - write formatted data to string
							sprintf(msg, "server: client %d just arrived\n", id);
							send_all(clients, total_clients, msg, connfd);
							// FD_SET - add fd to the set
							FD_SET(connfd, &cpyfds);
							// maxi - find the highest fd in the set and set it to max_fd this is needed for select
							maxi(clients, &max_fd, total_clients);
							printf("New client connected. ID: %d, FD: %d\n", id, connfd);
							// Break out of the loop to prevent double connections
							break ;
						}
					}
					else
					{
						// Initialize client id with -1 to check if it exists
						int id = -1;

						// Cycle through clients to find the one that sent a message
						for (int i = 0; i < total_clients; i++)
						{
							// If client exists and their fd matches the one that sent a message, set id to their id
							if (clients[i].fd == fd)
							{
								id = i;
								printf("Client %d exists, checking their fds for messsages", id);
								// Found the client, no need to cycle through the rest
								break ;
							}
						}

						// If client doesn't exist, skip
						if (id == -1)
							continue ;

						// If client exists, read their message
						char buffer[4096] = {'\0'};
						// MSG_DONTWAIT - non-blocking mode, so we can read all messages
						int readed = recv(fd, &buffer, 4096, MSG_DONTWAIT);
						// Add null terminator to the end of the message
						buffer[readed] = '\0';

						// If client disconnected, remove them from the list, close their fd and free their message buffer
						if (readed <= 0)
						{
							char msg[50] = {'\0'};
							// sprintf - write formatted data to string
							sprintf(msg, "server: client %d just left\n", id);
							send_all(clients, total_clients, msg, fd);
							// FD_CLR - remove fd from the set
							FD_CLR(fd, &cpyfds);
							// close - close file descriptor
							close(fd);
							// Set client fd to -1 to indicate that it's not in use
							clients[id].fd = -1;
							// Free message buffer to prevent memory leaks
							free(clients[id].message_buffer);
							printf("Client %d disconnected\n", id);
						}
						// If client sent a message, send it to all other clients except the sender
						else
						{
							// Initialize message to send
							char *msg_to_send = NULL;
							// str_join - join two strings, then free the first one
							clients[id].message_buffer = str_join(clients[id].message_buffer, buffer);
							// extract_message - looks for \n in the message and returns it if found
                            while (extract_message(&clients[id].message_buffer, &msg_to_send))
							{
								// Send messages in two parts:
                                char msg[50] = {'\0'};
								// First we send the client id
                                sprintf(msg, "client %d: ", id);
                                send_all(clients, total_clients, msg, fd);
								// Then we send the message we extracted
                                send_all(clients, total_clients, msg_to_send, fd);
								// Free the message we extracted
                                free(msg_to_send);
							}
						}
					}
				}
			}
		}
	}
}
