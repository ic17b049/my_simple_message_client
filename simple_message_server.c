#include <err.h>
#include <errno.h>
#include <getopt.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>


static void parse_params(int argc, char *argv[], char *port[]);
static int init_sock(char *port);
static int accept_connections(int sock);
static void sigchld_handler(int sig);
static void usage(FILE *stream, const char *cmnd, int exitcode);


int main(int argc, char *argv[]) {
	char *port = NULL;
	int sock = -1;

	parse_params(argc, argv, &port);



	if ((sock = init_sock(port)) == -1) {
		return EXIT_FAILURE;
	}

	if (accept_connections(sock) == -1) {
		return EXIT_FAILURE;
	}

	close(sock);

	return 0;
}

static void parse_params(int argc, char *argv[], char *port[]) {

	int c;

	if(argc < 2) usage(stdout, argv[0], EXIT_FAILURE );
	
	while (1) {
		int option_index = 0;
		
		static struct option long_options[] = {
			{"port",  required_argument, NULL,  'p' },
			{"help",  no_argument,       NULL,  'h' },
			{0,       0,                 0,     0 }
		};

		c = getopt_long(argc, argv, "p:h",
		long_options, &option_index);
		if (c == -1)
		break;
		if (c == '?')
		break;
		
		switch (c) {
		case 'p':
			*port = optarg;
			break;
		default:
			usage(stdout, argv[0], EXIT_FAILURE );
			break;
		}
	}
}

static int init_sock(char *port) {
	int sockfd = -1; // Std. Return
	struct addrinfo hints, *servinfo, *p;
	const int yes = 1;


	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	if (getaddrinfo(NULL, port, &hints, &servinfo) != 0) {
		return -1;
	}

	for (p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
			continue;
		}

		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
			close(sockfd);
			continue;
		}

		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			continue;
		} 
		
		break;
	}
	
	freeaddrinfo(servinfo);
	if (p == NULL) {
		warnx("bind");
		return -1;
	}
	return sockfd;
}

static int accept_connections(int sock) {
	int new_fd = -1;
	struct sockaddr_in addr;
	socklen_t addr_size = sizeof(struct sockaddr_in);

	struct sigaction sa;
	sa.sa_handler = sigchld_handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;

	if (sigaction(SIGCHLD, &sa, NULL) == -1) {
		warn("sigaction");
		close(sock);
		return -1;
	}

	if (listen(sock, SOMAXCONN) == -1) {
		warn("listen");
		close(sock);
		return -1;
	}

	while (1) {
		new_fd = accept(sock, (struct sockaddr *)&addr, &addr_size);
		if (new_fd == -1) {
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
				continue;
			} else {
				warn("accept");
				close(sock);
				return -1;
			}
		}

		switch (fork()) {

		case -1: //ERROR
			warn("fork");
			close(new_fd);
			break;

		case 0: //CHILD
			close(sock);
			if (dup2(new_fd, STDIN_FILENO) == -1) {
				warn("dup2 in");
				_exit(EXIT_FAILURE);
			}
			if (dup2(new_fd, STDOUT_FILENO) == -1) {
				warn("dup2 out");
				_exit(EXIT_FAILURE);
			}
			close(new_fd);
			execl("/usr/local/bin/simple_message_server_logic", "", NULL);
			//reaches only when execl fails
			warn("execl");
			_exit(EXIT_FAILURE);

		default: //PARENT
			close(new_fd);
			break;
		}
	}

	return 0;
}

static void sigchld_handler(int s) {
	(void)s;
	int saved_errno = errno;
	while(waitpid(-1, NULL, WNOHANG) > 0);
	errno = saved_errno;
}

static void usage(FILE *stream, const char *cmnd, int exitcode) {
	fprintf(stream, "usage: %s options\n", cmnd);
	fprintf(stream, "options:\n");
	fprintf(stream, "        -p, --port <port>\n");
	fprintf(stream, "        -h, --help\n");
	exit(exitcode);
}




