#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <simple_message_client_commandline_handling.h>
#include <unistd.h>

void usage(FILE *stream, const char *cmnd, int exitcode);
int sendall(int s, char *buf, int *len);
int connectToServer(const char *server, const char *port);

int main(const int argc, const char * const argv[])
{

	const char *server;
	const char *port;
	const char *user;
	const char *message;
	const char *img_url;
	int verbose;
	int sfd;

	smc_parsecommandline(argc, argv, usage, &server, &port, &user, &message,
	&img_url, &verbose);

	printf("Server:  %s\n", server);
	printf("Port:    %s\n", port);
	printf("User:    %s\n", user);
	printf("Message: %s\n", message);
	printf("Img_url: %s\n", img_url);
	printf("Verbose: %i\n", verbose);

	sfd = connectToServer(server, port);

	close(sfd);

	printf("SUCCCESSS\n");

	return 0;
}

int connectToServer(const char *server, const char *port)
{

	int sfd, s;
	struct addrinfo hints;
	struct addrinfo *result, *rp;

	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC; /* Allow IPv4 or IPv6 */
	hints.ai_socktype = SOCK_STREAM; /* Stream socket */

	s = getaddrinfo(server, port, &hints, &result);
	if (s != 0)
	{
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
		exit(EXIT_FAILURE);
	}

	for (rp = result; rp != NULL; rp = rp->ai_next)
	{
		sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if (sfd == -1)
		continue;
		if (connect(sfd, rp->ai_addr, rp->ai_addrlen) != -1)
		break; /* Success */

	}

	if (rp == NULL)
	{ /* No address succeeded */
		fprintf(stderr, "Could not connect\n");
		exit(EXIT_FAILURE);
	}

	freeaddrinfo(result); /* No longer needed */
	return sfd;
}

//https://cis.technikum-wien.at/documents/bic/3/vcs/download/vcs_tcpip/bgnet_A4.pdf Page 38
int sendall(int s, char *buf, int *len)
{
	int total = 0; // how many bytes we've sent
	int bytesleft = *len; // how many we have left to send
	int n;
	while (total < *len)
	{
		n = send(s, buf + total, bytesleft, 0);
		if (n == -1)
		{
			break;
		}
		total += n;
		bytesleft -= n;
	}
	*len = total; // return number actually sent here
	return n == -1 ? -1 : 0; // return -1 on failure, 0 on success
}

void usage(FILE *stream, const char *cmnd, int exitcode)
{
	fprintf(stream, "usage: %s options\n", cmnd);
	fprintf(stream, "options:\n");
	fprintf(stream, "        -s, --server <server>   full qualified domain name or IP address of the server\n");
	fprintf(stream, "        -p, --port <port>       well-known port of the server [0..65535]\n");
	fprintf(stream, "        -u, --user <name>       name of the posting user\n");
	fprintf(stream, "        -i, --image <URL>       URL pointing to an image of the posting user\n");
	fprintf(stream, "        -m, --message <message> message to be added to the bulletin board\n");
	fprintf(stream, "        -v, --verbose           verbose output\n");
	fprintf(stream, "        -h, --help\n");
	exit(exitcode);
}