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
#define MESSAGE_TYPE_RRQ 1
struct __attribute__((__packed__)) message_1RRQ {
unsigned char msg_type;
unsigned char win_size;
char filename[uFILENAME_SIZE];
};

// TYPE 2 - DATA
#define uDATASIZE 512
#define MESSAGE_TYPE_DATA 2
struct __attribute__((__packed__)) message_2DATA {
unsigned char msg_type;
unsigned char seq_no;
char data[uDATASIZE];
};

// TYPE 3 - ACK
#define MESSAGE_TYPE_ACK 3
struct __attribute__((__packed__)) message_3ACK {
unsigned char msg_type;
unsigned char seq_no;
};

// TYPE 4 - ERROR
#define MESSAGE_TYPE_ERROR 4
struct __attribute__((__packed__)) message_4ERROR {
unsigned char msg_type;
};


int validate_and_unpack (char* pmessage_buffer, int imessage_size, void* pdestination_message);


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
    struct sockaddr_in servaddr;
    char user_input[1000];
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
    printf("Wrote %d bytes\n", bytes_written);
    
    int bytes_gotten;

    struct message_3ACK ack_msg;
    ack_msg.msg_type = 3;
    struct message_4ERROR error_msg;

    int instruction;

    while (1)
    {

      fprintf(stderr, "0 to receive message, 1 to send ACK\n");
      fgets(user_input, 10000, stdin);
      instruction = atoi(user_input);
      fprintf(stderr, "Instruction received as%d\n", instruction);

      if (instruction == 0)
      {
        fprintf(stderr, "Waiting for message...\n");
        bytes_gotten = recvfrom(sockfd,recvline,10000,0,NULL,NULL);
        recvline[bytes_gotten] = 0;
        fprintf(stderr, "Got message size %d\n", bytes_gotten);
        validate_and_unpack(recvline, bytes_gotten, &error_msg);  
      } 
      if (instruction == 1)
      {
        fprintf(stderr, "Sending ACK... Select number to send:\n");
        fgets(user_input, 10000, stdin);
        instruction = atoi(user_input);
        fprintf(stderr, "Sending ack with number%u\n", instruction);
        ack_msg.seq_no = instruction;
        ack_msg.msg_type = MESSAGE_TYPE_ACK;
        bytes_written = sendto(sockfd,&ack_msg,sizeof(ack_msg),0,
                 (struct sockaddr *)&servaddr,sizeof(servaddr));
        printf("Wrote ACK of %d bytes with seq_no %u, message type %u\n", bytes_written, ack_msg.seq_no, ack_msg.msg_type);
      }
      

    }
    return 0;
}

#define uMAX_SEQ_NUM 50

int validate_and_unpack (char* pmessage_buffer, int imessage_size, void* pdestination_message) 
{
  unsigned char type_gotten = pmessage_buffer[0];
  switch (type_gotten){
    case MESSAGE_TYPE_RRQ:
      fprintf(stderr, "Validating/Unpacking message of type 1 - RRQ\n");
      if (imessage_size < 4 || imessage_size > 22) 
      {
        //Not possible - message needs to contain type, win size, filename
        fprintf(stderr, "ERROR: Impossible initial message size, %d\n", imessage_size);
        return -1;
      }
      struct message_1RRQ* pRRQ_msg_unpacked = pdestination_message;
      pRRQ_msg_unpacked->msg_type = pmessage_buffer[0];
      if (pRRQ_msg_unpacked->msg_type != MESSAGE_TYPE_RRQ) 
      {
        fprintf(stderr, "ERROR: Initial message type was %u\n", pRRQ_msg_unpacked->msg_type);
        return -1;
      }
      fprintf(stderr, "Message type is %u\n", pRRQ_msg_unpacked->msg_type);

      pRRQ_msg_unpacked->win_size = pmessage_buffer[1];
      if (pRRQ_msg_unpacked->win_size < 1 || pRRQ_msg_unpacked->win_size > 9) 
      {
        fprintf(stderr, "ERROR: Window size was %u\n", pRRQ_msg_unpacked->win_size);
        return -1;
      }
      fprintf(stderr, "Window size requested is %u\n", pRRQ_msg_unpacked->win_size);
      // total length of message minus 2 bytes for ^
      int ifilename_len = imessage_size - 2;
      strncpy(pRRQ_msg_unpacked->filename, &(pmessage_buffer[2]), ifilename_len); 
      fprintf(stderr, "Filename parsed as %s\n", pRRQ_msg_unpacked->filename);
      // Good to go!
      return 1;
    
    case MESSAGE_TYPE_DATA:
      fprintf(stderr, "Received DATA message.\n");
      struct message_2DATA* pDATA_unpacked = pdestination_message;
      pDATA_unpacked->msg_type = pmessage_buffer[0];
      fprintf(stderr, "Message type was %u\n", pDATA_unpacked->msg_type);
      pDATA_unpacked->seq_no = pmessage_buffer[1];
      fprintf(stderr, "Seq number was %u\n", pDATA_unpacked->seq_no);
      bzero(&(pDATA_unpacked->data), uDATASIZE);
      memcpy(&(pDATA_unpacked->data), &(pmessage_buffer[2]), imessage_size-2);
      //fprintf(stderr, "Data was %s\n", pDATA_unpacked->data);
      return -1;

    case MESSAGE_TYPE_ACK:
      fprintf(stderr, "Validating/Unpacking message of type 3 - ACK\n");
      if (imessage_size != 2) 
      {
        //Not possible - message needs to contain type, and seq_no
        fprintf(stderr, "ERROR: Impossible initial message size, %d\n", imessage_size);
        return -1;
      }
      struct message_3ACK* pACK_msg_unpacked = pdestination_message;
      pACK_msg_unpacked->msg_type = pmessage_buffer[0];
      if (pACK_msg_unpacked->msg_type != MESSAGE_TYPE_ACK) 
      {
        fprintf(stderr, "ERROR: Initial message type was %u\n", pACK_msg_unpacked->msg_type);
        return -1;
      }
      fprintf(stderr, "Message type is %u\n", pACK_msg_unpacked->msg_type);
      pACK_msg_unpacked->seq_no = pmessage_buffer[1];
      if (pACK_msg_unpacked->seq_no > uMAX_SEQ_NUM) 
      {
        fprintf(stderr, "ERROR: Invalid sequence number %d\n", pACK_msg_unpacked->seq_no);
        return -1;
      }
      fprintf(stderr, "Sequence number is %u\n", pACK_msg_unpacked->seq_no);
      // Good to go!
      return 1;
    
    case MESSAGE_TYPE_ERROR:
      fprintf(stderr, "Received ERROR message of type 4.\n");
      return 0;
  }
  return -1;
}


