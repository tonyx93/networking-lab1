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

#define PORT 3456    //Specifies which port to listen on
#define MSGLEN 1024   //Max message length to be sent at a time
#define QUELEN 10    //Max queue length of connecting clients

int main()
{
  int sockfd;
  struct sockaddr_in server;
  char buffer[MSGLEN];
  
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

  //Server running...
  printf("Server is now listening for connections\n");
  while(1)
    {
      int clientfd;
      struct sockaddr_in client;
      int clientlen = sizeof(client);
      
      //Accept a connection
      clientfd = accept(sockfd,(struct sockaddr *)&client, &clientlen);
      printf("%s:%d connected\n", inet_ntoa(client.sin_addr), ntohs(client.sin_port));

      /* BEGIN HTTP */
 
      char *line;
      char *word;
      recv(clientfd, buffer, MSGLEN, 0);      // read client request
      //printf(buffer);
      line = strtok(buffer,"\n"); //Get request line
      word = strtok(line," \r"); //Parse the method type
      if(strcmp(word,"GET")==0) //GET
	{
	  char *res;
	  char *version;
	  res = strtok(line," \r"); //Parse resource
	  version = strtok(line," \r");//Parse version
	  line = strtok(buffer,"\n"); //Parse Response Headers
	  while(strlen(line)>1) //Will exit loop when line is an empty line
	    {
	      //I don't know what I'm supposed to do with the headers
	      line = strtok(buffer,"\n");
	    }
	  //At this point, buffer contains the optional message body

	  //Server response for GET
	  FILE *file;
	  file = fopen(strcat(".",res),"r");
	  char buf2[1000]; //Another buffer for storing bytes
	  struct tm timeinfo; //For time stuff
	  time_t rawtime;
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
	  else
	    {
	      struct stat fstat; //Get file stats
	      stat(strcat(".",res),&fstat);

	      sprintf(buffer,"HTTP/1.1 200 OK\r\n"); //Status Line
	      strcat(buffer,"Date: "); //Date header
	      time(&rawtime);
	      timeinfo = *gmtime(&rawtime);
	      strftime(buf2,sizeof(buf2), "%a, %d %b %Y %H:%M:%S %Z",&timeinfo);
	      strcat(buffer,buf2);
	      strcat(buffer,"\r\n");
	      strcat(buffer,"Last-Modified: "); //Last Modified header
	      timeinfo = *gmtime(&(fstat.st_mtime));
	      strftime(buf2,sizeof(buf2), "%a, %d %b %Y %H:%M:%S %Z",&timeinfo);
	      strcat(buffer,buf2);
	      strcat(buffer,"/r/n");
	      strcat(buffer,"Content-Type: text/html; charset=UTF-8\r\n"); //Content Type header
	      strcat(buffer,"Content-Length: "); //Content Length header
	      sprintf(buf2,"%lld",fstat.st_size);
	      strcat(buffer,buf2);
	      strcat(buffer,"\r\n");
	      strcat(buffer,"Connection: close\r\n\r\n"); //Connection header and empty line
	      
	      fread(buf2,1,fstat.st_size,file);
	      strcat(buffer,buf2);
	    }
	  send(clientfd, buffer, MSGLEN, 0);
	}
      /* END HTTP */
      
      //Close connection
      close(clientfd);
    }

  //Clean up
  close(sockfd);
  return 0;
}
