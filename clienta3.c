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

//
// MESSAGE STRUCTS PER TYPE
//
// TYPE 1 - RRQ
#define uFILENAME_SIZE 20
struct __attribute__((__packed__)) message_1RRQ {
unsigned char msg_type;
unsigned char win_size;
char filename[uFILENAME_SIZE];
};

// TYPE 2 - DATA
#define uDATASIZE 512
struct __attribute__((__packed__)) message_2DATA {
unsigned char msg_type;
unsigned char seq_no;
char data[uFILENAME_SIZE];
};

// TYPE 3 - ACK
struct __attribute__((__packed__)) message_3ACK {
unsigned char msg_type;
unsigned char seq_no;
};

// TYPE 4 - ERROR
struct __attribute__((__packed__)) message_4ERROR {
unsigned char msg_type;
};



int main(int argc, char**argv)
{
  int portno;    
  //Check that port is provided
  if (argc < 4) 
  {
    fprintf(stderr, "%s\n", "Usage: testclient <port> <filename> <window_size>");
    exit(1);
  }
  //Grab port number
    portno = atoi(argv[1]);

    int sockfd, bytes_written;
    struct sockaddr_in servaddr,cliaddr;
    char sendline[1000];
    char recvline[1000];

    sockfd=socket(AF_INET,SOCK_DGRAM,0);

    bzero(&servaddr,sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr=inet_addr("127.0.0.1");
    servaddr.sin_port=htons(portno);

    //Send request for filename
    char filename[20];
    strcpy(filename, argv[2]);
    fprintf(stderr, "Requesting file named %s\n", filename);

    //Get window number
    int window_size;
    window_size = atoi(argv[3]);
    fprintf(stderr, "With window size %u\n", window_size);

    struct message_1RRQ request_msg;
    request_msg.msg_type = 1;
    strcpy(request_msg.filename, filename);
    request_msg.win_size = window_size;
    printf("Sending RRQ!\n");
      
    bytes_written = sendto(sockfd,&request_msg,sizeof(request_msg),0,
             (struct sockaddr *)&servaddr,sizeof(servaddr));
      

    while (fgets(sendline, 10000,stdin) != NULL)
    {



      printf("Wrote %d bytes\n", bytes_written);
      bytes_written = recvfrom(sockfd,recvline,10000,0,NULL,NULL);
      recvline[n]=0;
      fputs(recvline,stdout);
    }
    return 0;
}



