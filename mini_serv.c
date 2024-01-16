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

int cl_fd[4096] = {-1};
int cl_id_fd[4096] = {-1};

char *extract_id[4096] = {0};

void erro_msg(char *msg)
{
	write(2, msg, strlen(msg));
	exit(1);
}

void maxi()
{ 
	for (int i = 0; i < client_id; i++)
	{
		if (cl_fd[i] > max_fd)
			max_fd = cl_fd[i];
	}
}

void send_all(char *msg, int fd)
{ 
	for (int i = 0; i < client_id; i++)
	{
		if (cl_fd[i] != fd && cl_fd[i] != -1)
			send(cl_fd[i], msg, strlen(msg), 0);
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

	int sockfd, connfd;
	socklen_t len;
	struct sockaddr_in servaddr, cli; 

	sockfd = socket(AF_INET, SOCK_STREAM, 0); 
	if (sockfd == -1)
		erro_msg("Fatal error\n");

	bzero(&servaddr, sizeof(servaddr)); 

	fd_set readfds, writefds, cpyfds;

 	FD_ZERO(&cpyfds);
	FD_SET(sockfd, &cpyfds);

	max_fd = sockfd;

	servaddr.sin_family = AF_INET; 
	servaddr.sin_addr.s_addr = htonl(2130706433);
	servaddr.sin_port = htons(atoi(argv[1])); 

	if ((bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr))) != 0)
		erro_msg("Fatal error\n");
	if (listen(sockfd, 10) != 0)
		erro_msg("Fatal error\n");

	while (1)
	{
		readfds=writefds=cpyfds;
		if (select(max_fd + 1, &readfds, &writefds,0 , 0) < 0) {
		}
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
						} 
						else
						{
							int id = client_id++;
							cl_id_fd[connfd] = id;
							cl_fd[id] = connfd;
							char msg[50] = {'\0'};
							sprintf(msg, "server: client %d just arrived\n", id);
							send_all(msg, connfd);
							FD_SET(connfd, &cpyfds);
							maxi();
							break ;
						}
					}
					else
					{
						int id = cl_id_fd[fd];
						char buffer[4096] = {'\0'};
						int readed = recv(fd, &buffer, 4096, MSG_DONTWAIT);
						buffer[readed] = '\0';
						if (readed <= 0)
						{
							char msg[50] = {'\0'};
							sprintf(msg, "server: client %d just left\n", id);
							send_all(msg, fd);
							FD_CLR(fd, &cpyfds);
							close(fd);
							cl_fd[id] = -1;
							free(extract_id[id]);
						}
						else
						{
							char *msg_to_send = 0;
							extract_id[id] = str_join(extract_id[id], buffer);
							while (extract_message(&extract_id[id], &msg_to_send))
							{
								char msg[50] = {'\0'};
								sprintf(msg, "client %d: ", id);
								send_all(msg, fd);
								send_all(msg_to_send, fd);
								free(msg_to_send);
							}
						}
					}
				}
			}
		}
	}
}
