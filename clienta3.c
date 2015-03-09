/*
A UDP client that facilitates a file transfer using the Go-Back-N protocol


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
   int sockfd,n;
   struct sockaddr_in servaddr,cliaddr;
   char sendline[1028];
   char recvline[1028];


   if (argc != 2)
   {
      printf("usage: udpcli <port>\n");
      exit(1);
   }
   
   unsigned short port = atoi(argv[1]);

   sockfd=socket(AF_INET,SOCK_DGRAM,0);

   bzero(&servaddr,sizeof(servaddr));
   servaddr.sin_family = AF_INET;
   servaddr.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
   servaddr.sin_port=htons(port);

   while (fgets(sendline, 1028,stdin) != NULL)
   {
      fprintf(stderr, "Sending %s\n", sendline);
      sendto(sockfd,sendline,strlen(sendline),0,
             (struct sockaddr *)&servaddr,sizeof(servaddr));
      n=recvfrom(sockfd,recvline,1028,0,NULL,NULL);
      recvline[n]=0;
      fputs(recvline,stdout);
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


