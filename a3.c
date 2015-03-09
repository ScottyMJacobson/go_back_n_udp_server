/*
A UDP server that facilitates a file transfer using the Go-Back-N protocol


// My ports are 9540-9559
*/

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

//
// SOCKET TYPEDEF AND FORWARD DECLARATION
//
typedef int sockfd_t;
sockfd_t socket_factory (uint16_t port);


int main(int argc, char**argv)
{
   int portno;    
   //Check that port is provided
   if (argc < 2) 
   {
     fprintf(stderr, "%s\n", "ERROR: No port provided");
     exit(1);
   }
   //Grab port number
   portno = atoi(argv[1]);

   //Init variables we'll need
   sockfd_t sockfd;
   fd_set gloabl_fd_set, temp_fd_set;
   int iconnection;
   struct sockaddr_in cli_addr;
   socklen_t clilen;

   char message_buffer[1028];

   sockfd = socket_factory(portno);

   int bytes_read = 0;
   int bytes_written = 0;

   for (;;)
   {
      clilen = sizeof(cli_addr);
      bytes_read = recvfrom(sockfd,message_buffer,1028,0,(struct sockaddr *)&cli_addr,&clilen);
      fprintf(stderr, "Read something!\n" );
      sendto(sockfd,message_buffer,bytes_read,0,(struct sockaddr *)&cli_addr,sizeof(cli_addr));
      printf("-------------------------------------------------------\n");
      message_buffer[bytes_read] = 0;
      printf("Received the following:\n");
      printf("%s",bytes_read);
      printf("-------------------------------------------------------\n");
   }
}



/*////////////////////////////////////////////////////////////////////////////

_____________  ____________ 
[__ |  ||   |_/ |___ | [__  
___]|__||___| \_|___ | ___] 
                            
*/
//Takes in port number, returns a sockfd_t file descriptor representing 
//  the server socket it creates
sockfd_t socket_factory (uint16_t port)
{
  sockfd_t retfd;
  struct sockaddr_in serv_addr;

  //Create the socket and make sure it didn't fail
  retfd = socket (AF_INET, SOCK_DGRAM, 0);
  if (retfd < 0)
  {
    perror ("ERROR: Failed to create socket");
    exit (EXIT_FAILURE);
  }

  //Set socket serv_addr attributes and set port
  bzero(&serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons (port);
  serv_addr.sin_addr.s_addr = htonl (INADDR_ANY);
  if (bind (retfd, (struct sockaddr *) &serv_addr, sizeof (serv_addr)) < 0)
  {
    perror ("ERROR: Failed to bind socket with specified settings");
    exit (EXIT_FAILURE);
  }

  return retfd;
}



