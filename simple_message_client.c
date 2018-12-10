#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <simple_message_client_commandline_handling.h>
#include <unistd.h>


#define verb(...)                                                                                          \
  if (verbose) {                                                                                              \
    printf("%s [%s,%s(), line %i]: ", prgname, __FILE__, __func__, __LINE__);\
	printf(__VA_ARGS__);\
    printf("\n");\
  }


#define err(...) fprintf(stderr, "%s: ", prgname);fprintf(stderr, __VA_ARGS__);fprintf(stderr, "\n");\

  

static void usage(FILE *stream, const char *cmnd, int exitcode);
static int connectToServer(const char *server, const char *port);

int verbose;
const char *prgname;


int main(const int argc, const char * const argv[])
{
    prgname = argv[0];

	const char *server, *port, *user, *message, *img_url;
	int sfd;
	
	
	FILE* fpw;
	FILE* fpr;
	char *line = NULL;
	size_t len = 0;
	ssize_t read;
	
	FILE* fpwNewFile2 = NULL;

	int statusResp = 0;
	
	smc_parsecommandline(argc, argv, usage, &server, &port, &user, &message,&img_url, &verbose);
    verb("Using the following options: server=\"%s\" port=\"%s\", user=\"%s\", img_url=\"%s\", message=\"%s\"", server, port, user, img_url, message );
	
	sfd = connectToServer(server, port);

	fpw = fdopen(sfd, "w");
	
	
	if(fpw == NULL){
		err("fdopen failed");
	}
	
	
	fprintf(fpw, "user=%s\n", user);
	if(img_url != NULL) fprintf(fpw, "img=%s\n", img_url);
	fprintf(fpw, "%s", message);
	fflush(fpw);	
	if(shutdown(sfd, SHUT_WR) == -1){
		fclose(fpw);
		err("shutdown failed");
	}
	
	fpr = fdopen(sfd, "r");	
	if(fpw == NULL){
		fclose(fpw);
		close(sfd);
		err("fdopen failed");
	}	
	
	int currentState = 0;
	while(currentState !=-1){
		char *ptr;
		read = getline(&line, &len, fpr);
		if(line[read-1]== '\n') line[read-1]= '\0';

		if(read == -1){
			currentState = -1; // EOF
		}else{

			ptr = strtok(line, "=");
			
			if(strcmp(ptr, "status") == 0){
				ptr = strtok(NULL, "=");
				char *endptr;
				statusResp  = strtol(ptr, &endptr, 10);
				if(*endptr != '\0'){
						err("wrong status Format");					
				}				
			}
			
			if(strcmp(ptr, "file") == 0){
				ptr = strtok(NULL, "=");	
				fpwNewFile2 = fopen(ptr, "w");
					if(fpwNewFile2 == NULL){
						fclose(fpr);
						fclose(fpw);
						close(sfd);
						err("fopen failed");
					}
			}

			if(strcmp(ptr, "len") == 0){
				ptr = strtok(NULL, "=");	
				char *endptr;
				long fileSize = strtol(ptr, &endptr, 10);
				if(*endptr != '\0'){
						fclose(fpwNewFile2);
						fclose(fpr);
						fclose(fpw);
						close(sfd);
						err("wrong filesize");					
				}
				
				
				while(fileSize != 0){
					int bufferSize = 255;
					char buff[bufferSize];
					int tmpReadSize = bufferSize;
					if(fileSize < bufferSize){
						tmpReadSize = fileSize;
					}
					
					int dataRead = fread(buff, sizeof(char) ,tmpReadSize, fpr);
					fileSize = fileSize - dataRead;
					
					fwrite(buff, sizeof(char) ,dataRead, fpwNewFile2);

				}

				fflush(fpwNewFile2);
				fclose(fpwNewFile2);


			}
			
		}
		
	}
	
	free(line);
	close(sfd);


	return statusResp;
}

static int connectToServer(const char *server, const char *port)
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
		err("getaddrinfo: %s\n", gai_strerror(s));
		freeaddrinfo(result);
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


	    err("Could not connect to Server")
		freeaddrinfo(result); 
		exit(EXIT_FAILURE);
	}
	verb("Connected to Server");
	freeaddrinfo(result); /* No longer needed */
	return sfd;
}


static void usage(FILE *stream, const char *cmnd, int exitcode)
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
