#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <signal.h>

#define PORT 3456    //Specifies which port to listen on
#define MSGLEN 2048   //Max message length to be sent at a time
#define QUELEN 10    //Max queue length of connecting clients

int sockfd;
FILE *file;

void quit(int sig) {
	// Clean up
	printf("Server is shutting down\n");
	close(sockfd);
	
	// Exit program
	exit(sig);
}

/*
	Notes:

   struct sockaddr_in {
	   short int			sin_family; // Address family, AF_INET
	   unsigned short int	sin_port; // Port number
	   struct in_addr;		sin_addr; // Internet address
	   unsigned char;		sin_zero[8]; // Same size as struct sockaddr
   };

*/
int main()
{
	struct sockaddr_in server;
	char buffer[MSGLEN];
	pid_t pid;
  
	// Create a file descriptor for a socket
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("Error creating file descriptor for socket");
		exit(errno);
	}

	// Fill in information fields for creating a custom socket
	memset(&server, 0, sizeof(server));
	server.sin_family = AF_INET;
	server.sin_port = htons(PORT);
	server.sin_addr.s_addr = INADDR_ANY;
  
	// Physically open a socket in machine, bind to socket descriptor sockfd
	if (bind(sockfd, (struct sockaddr *)&server, sizeof(server)) != 0)
	{
		perror("Error binding");
		exit(errno);
    }

	//Start listening on the socket
	if (listen(sockfd,QUELEN) != 0)
	{
		perror("Error listening");
		exit(errno);
    }

	signal(SIGINT, quit);
	//Server running...
	printf("Server is now listening for connections\n");
	printf("Press CTRL-c to stop the server\n");
	while(1)
	{
		int clientfd;
		struct sockaddr_in client;
		int clientlen = sizeof(client);

		//Accept a connection
		clientfd = accept(sockfd,(struct sockaddr *)&client, &clientlen);
		printf("%s:%d connected\n", inet_ntoa(client.sin_addr), ntohs(client.sin_port));
		pid = fork();

		/* Fork child process to service connection */
		if (pid == 0) {

			// Close the server socket to process the request

			if ( close(sockfd) < 0 ) {
				perror("Error closing server socket in child process serving client.");
				exit(errno);
			}

			// Service the request
			char *word;
			char *line;
			char buf2[256]; //Another buffer for storing bytes
			memset(buffer,0,MSGLEN);
			memset(buf2,0,256);

			// Read client request
			recv(clientfd,buffer,MSGLEN,0);
			// Output request header to console
			printf("Client request:\n%s\n",buffer);

			word = strtok(buffer," "); //Parse the method type
			if(strcmp(word,"GET")==0) //GET
			{
				char *res;
				char *version;
				res = strtok(NULL," "); //Parse resource
				//printf("Resource: %s\n",res);
				version = strtok(NULL,"\r");//Parse version
				strtok(NULL,"\n");
				//printf("Version: %s\n",version);
				strtok(NULL,"\n");
				line = strtok(NULL,"\n"); //Parse Response Headers
				while(strlen(line)>1) //Will exit loop when line is an empty line
				{
					//I don't know what I'm supposed to do with the headers
					//line contains the header information so manipulate that string
				  
				  //print header
				        //printf("%s\n",line);
					line = strtok(NULL,"\n");
				}
				//At this point, buffer contains the optional message body
		  
				//Server response for GET
				char *directory;
				struct tm timeinfo; //For time stuff
				time_t rawtime;
				buf2[0] = '.';
				directory = strcat(buf2,res);
				file = fopen(directory,"r");
				memset(buffer,0,MSGLEN);
				memset(buf2,0,256);

				if(file==NULL)
				{
					sprintf(buffer,"HTTP/1.1 404 Not Found\r\n"); //File not found
					strcat(buffer,"Date: "); //Date header
					time(&rawtime);
					timeinfo = *gmtime(&rawtime);
					strftime(buf2,sizeof(buf2), "%a, %d %b %Y %H:%M:%S %Z",&timeinfo);
					strcat(buffer,buf2);
					strcat(buffer,"\r\n");
					//Other headers can go here
					strcat(buffer,"Connection: close\r\n\r\n");
				}
				else {
					struct stat fstat; //Get file stats
					stat(directory,&fstat);


					sprintf(buffer,"HTTP/1.1 200 OK\r\n"); //Status Line
					strcat(buffer,"Date: "); //Date header
					time(&rawtime);
					timeinfo = *gmtime(&rawtime);
					strftime(buf2,sizeof(buf2), "%a, %d %b %Y %H:%M:%S %Z",&timeinfo);
					strcat(buffer,buf2);
					strcat(buffer,"\r\n");

					strcat(buffer,"Last-Modified: "); //Last-Modified header
					timeinfo = *gmtime(&(fstat.st_mtime));
					strftime(buf2,sizeof(buf2), "%a, %d %b %Y %H:%M:%S %Z",&timeinfo);
					strcat(buffer,buf2);
					strcat(buffer,"\r\n");
					strcat(buffer,"Content-Type: text/html\r\n"); //Content Type header
					strcat(buffer,"Content-Length: "); //Content Length header

					sprintf(buf2,"%lld",fstat.st_size);
					strcat(buffer,buf2);
					strcat(buffer,"\r\n");
					strcat(buffer,"Connection: close\r\n\r\n"); //Connection header and empty line

					fread(buf2,1,fstat.st_size,file);
					strcat(buffer,buf2);
					fclose(file);
				}
			}

			printf("Server response:\n%s\n",buffer);
			send(clientfd, buffer, MSGLEN, 0);

			// Close client connection and exit child process
			if ( close(clientfd) < 0 ) {
				perror("Error closing client connection.");
				exit(errno);
			}
			exit(0);
		}

		
		/* Still in parent process */
		  
		// Close the connected socket so a new connection can be accepted
		if ( close(clientfd) < 0 ) {
			perror("Error closing client connection in parent.");
			exit(errno);
		}

		waitpid(-1, NULL, WNOHANG);
	}

	return 0;
}
