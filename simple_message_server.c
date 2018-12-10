#include <err.h>
#include <errno.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#define SERVER_LOGIC_PATH "/usr/local/bin/simple_message_server_logic"

static int parse_params(int argc, char *argv[], char *port[]);
static int init_sock(char *port);
static int accept_connections(int sock);
static void sigchild_handler(int sig);

/**
 * @brief entry point
 *
 * @param argc the number of arguments
 * @param argv the arguments
 *
 * @returns EXIT_SUCCESS, EXIT_FAILURE
 */
int main(int argc, char *argv[]) {
  char *port = NULL;
  int sock = -1;

  if (parse_params(argc, argv, &port) == -1) {
    /* error is printed by parse_params() */
    fprintf(stderr, "Usage: %s -p port [-h]\n", argv[0]);
    return EXIT_FAILURE;
  }

  if ((sock = init_sock(port)) == -1) {
    /* error is printed by init_sock() */
    return EXIT_FAILURE;
  }

  if (accept_connections(sock) == -1) {
    /* error is printed by accept_connections() */
    return EXIT_FAILURE;
  }

  close(sock);

  return 0;
}

/**
 * @brief parses commandline parameters
 *
 * @param argc the number of arguments
 * @param argv the arguments
 * @param port where to save the port
 *
 * @returns 0 if everything went well or -1 in case of error
 */
static int parse_params(int argc, char *argv[], char *port[]) {
  int opt;
  long port_num;
  char *notconv;

  if (argc < 2) {
    warnx("Arguments missing");
    return -1;
  }

  while ((opt = getopt(argc, argv, "p:h")) != -1) {
    switch (opt) {

    case 'p':
      errno = 0;
      port_num = strtol(optarg, &notconv, 10);
      if (errno != 0 ||       /* over-/underflow */
          *notconv != '\0' || /* invalid characters */
          port_num < 1 ||     /* port number out of range */
          port_num > 65535) {
        warnx("Invalid port number");
        return -1;
      }
      *port = optarg;
      break;

    case 'h':
      return -1;

    default:
      /* error is printed by getopt() */
      return -1;
    }
  }

  if (optind < argc) {
    warnx("Non-option arguments present");
    return -1;
  }

  return 0;
}

/**
 * @brief creates a socket and binds to it
 *
 * @param port the port number
 *
 * @returns the socket descriptor or -1 in case of error
 */
static int init_sock(char *port) {
  int sock = -1;
  struct addrinfo hints;
  struct addrinfo *info, *p;
  const int reuseaddr = 1;

  /* get the address info */
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;       /* IPv4 */
  hints.ai_socktype = SOCK_STREAM; /* TCP */
  hints.ai_flags = AI_PASSIVE;     /* Wildcard IP */

  if (getaddrinfo(NULL, port, &hints, &info) != 0) {
    warn("getaddrinfo");
    return -1;
  }

  /*
   * getaddrinfo() returns a list of address structures
   * try each address until we successfully bind
   */
  for (p = info; p != NULL; p = p->ai_next) {
    if ((sock = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
      warn("socket");
      continue;
    }

    /* allow the address to be reused after a crash */
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuseaddr, sizeof(int)) == -1) {
      warn("setsockopt");
      close(sock);
      continue;
    }

    if (bind(sock, p->ai_addr, p->ai_addrlen) == -1) {
      warn("bind");
      close(sock);
      continue;
    } else {
      break; /* success */
    }
  }

  if (p == NULL) {
    warnx("Could not bind");
    freeaddrinfo(info);
    return -1;
  }

  freeaddrinfo(info);

  return sock;
}

/**
 * @brief a forking server
 *
 * @param sock the server socket
 *
 * @returns 0 if everything went well or -1 in case of error
 */
static int accept_connections(int sock) {
  int accept_sock = -1;
  struct sockaddr_in addr;
  socklen_t addr_size = sizeof(struct sockaddr_in);

  /* set up a signal handler */
  struct sigaction sa;
  sa.sa_handler = sigchild_handler;
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
    if ((accept_sock = accept(sock, (struct sockaddr *)&addr, &addr_size)) == -1) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        continue;
      } else {
        warn("accept");
        close(sock);
        return -1;
      }
    }

    switch (fork()) {

    case -1: /* error */
      warn("fork");
      close(accept_sock);
      break;

    case 0: /* child */
      close(sock);
      if (dup2(accept_sock, STDIN_FILENO) == -1) {
        warn("dup2 in");
        _exit(EXIT_FAILURE);
      }
      if (dup2(accept_sock, STDOUT_FILENO) == -1) {
        warn("dup2 out");
        _exit(EXIT_FAILURE);
      }
      close(accept_sock);
      execl(SERVER_LOGIC_PATH, "", NULL);
      /* reached only if execl() failed */
      warn("execl");
      _exit(EXIT_FAILURE);

    default: /* parent */
      close(accept_sock);
      break;
    }
  }

  return 0;
}

/**
 * @brief handles SIGCHLD by waiting for dead processes
 *
 * @param sig the signal number (ignored)
 */
static void sigchild_handler(int sig) {
  (void)sig;
  while (waitpid(-1, NULL, WNOHANG) > 0);
}