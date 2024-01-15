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

int client_id = 0;

int max_fd = 0;

int client_fd[4096] = {-1};
int client_id_by_fd[4096] = {-1};

char *extract_id[4096] = {0};


void print_error(char *msg)
{
    write(2, msg, strlen(msg));
    exit(1);
}

void max_index()
{
    for (int i = 0; i < client_id; i++)
    {
        if (client_fd[i] > max_fd)
            max_fd = client_fd[i];
    }
}

void send_all(char *msg, int fd)
{
    for (int i = 0; i < client_id; i++)
    {
        if (client_fd[i] != fd && client_fd[i] != -1)
            send(client_fd[i], msg, strlen(msg), 0);
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
        print_error("Wrong number of arguments\n");
    
    int         sockfd, connfd;
    socklen_t   len;
	struct sockaddr_in servaddr, cli; 

	// socket create and verification 
	sockfd = socket(AF_INET, SOCK_STREAM, 0); 
	if (sockfd == -1)
        print_error("Fatal error\n");

	bzero(&servaddr, sizeof(servaddr)); 

	// assign IP, PORT 
	servaddr.sin_family = AF_INET; 
	servaddr.sin_addr.s_addr = htonl(2130706433); //127.0.0.1
	servaddr.sin_port = htons(atoi(argv[1]));

    fd_set readfds, writefds, allfds;

    FD_ZERO(&allfds);
    FD_SET(sockfd, &allfds);

    max_fd = sockfd;

	// Binding newly created socket to given IP and verification 
	if ((bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr))) != 0)
        print_error("Fatal error\n");

    if (listen(sockfd, 10) != 0)
        print_error("Fatal error\n");

    while (1)
    {
        readfds = writefds = allfds;
        if (select(max_fd + 1, &readfds, &writefds, 0, 0) == -1)
            continue;
        else
        {
            for (int fd = sockfd; fd <= max_fd; fd++)
            {
                if (FD_ISSET(fd, &readfds))
                {
                    if (fd == sockfd)
                    {
                        len = sizeof(cli);
                        connfd = accept(sockfd, (struct sockaddr *)&cli, &len);
                        if (connfd < 0) { 
                            print_error("Fatal error\n");
                        } 
                        else
                        {
                            client_id_by_fd[connfd] = client_id;
                            client_fd[client_id] = connfd;
                            char msg[4096] = {'\0'};
                            sprintf(msg, "server: client %d just arrived\n", client_id);
                            send_all(msg, connfd);
                            FD_SET(connfd, &allfds);
                            max_index();
                            client_id++;
                            break ;
                        }
                    }
                    else
                    {
                        int id = client_id_by_fd[fd];
                        char buffer[4096] = {'\0'};
                        int ret = recv(fd, buffer, 4096, 0);
                        buffer[ret] = '\0';
                        if (ret == 0)
                        {
                            char msg[4096] = {'\0'};
                            sprintf(msg, "server: client %d just left\n", id);
                            send_all(msg, fd);
                            FD_CLR(fd, &allfds);
                            client_fd[id] = -1;
                            client_id_by_fd[fd] = -1;
                            close(fd);
                        }
                        else
                        {
                            char *msg_to_send = 0;
                            extract_id[id] = str_join(extract_id[id], buffer);
                            while (extract_message(&extract_id[id], &msg_to_send))
                            {
                                char msg[4096] = {'\0'};
                                sprintf(msg, "client %d: ", id);
                                strcat(msg, msg_to_send);
                                send_all(msg, fd);
                                free(msg_to_send);
                            }
                        }

                    }
                }
            }
        }
    }
}
