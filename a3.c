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
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

#define uBUFFER_SIZE 1028
#define uBABY_BUFFER 4
#define uTIMEOUT_US 3000
#define uTIMEOUT_SEC 0

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


//
// STRUCT TO KEEP TRACK OF SERVER STATE
//
struct server_state {
  char filename[uFILENAME_SIZE];
  int win_size;
  int final_window_bottom;
  int this_is_final_window;
  int file_size;
  int max_seq_no;
  int final_seq_size;
  int last_seq_sent;
  int recent_ACK_received;
  int transfer_started;
  int consecutive_timeouts;
  struct sockaddr_in* cli_addr;
  socklen_t clilen;
  sockfd_t sockfd;
  FILE* pfile_to_transfer;
};
void reset_state(struct server_state* pstate) {
  bzero(&(pstate->filename), uFILENAME_SIZE);
  pstate->win_size = 0;
  pstate->final_window_bottom = 0;
  pstate->this_is_final_window = 0;
  pstate->file_size = 0;
  pstate->max_seq_no = 0;
  pstate->final_seq_size = 0;
  //sentinel value meaning none sent/received yet
  pstate->last_seq_sent = -1;
  pstate->recent_ACK_received = -1;
  pstate->transfer_started = 0;
  pstate->consecutive_timeouts = 0;
  pstate->cli_addr = NULL;
  pstate->clilen = 0;
  pstate->sockfd = 0;
  pstate->pfile_to_transfer = NULL;
}

//
// MESSAGE BEGIN_TRANSFER FORWARD DECLARATION
//
int begin_transfer(char* message_buffer, int message_size, struct server_state* pserv_state);


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

  sockfd_t sockfd;
  int bytes_read = 0;
  struct sockaddr_in serv_addr,cli_addr;
  socklen_t clilen;
  char temp_buffer[uBUFFER_SIZE];

  sockfd=socket(AF_INET,SOCK_DGRAM,0);
  if (sockfd < 0) 
  {
    fprintf(stderr, "ERROR: Couldn't create socket.\n");
    exit(1);
  }

  bzero(&serv_addr,sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr=htonl(INADDR_ANY);
  serv_addr.sin_port=htons(portno);
  if (bind(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0) 
  {
    fprintf(stderr, "ERROR: Couldn't bind socket.\n");
    exit(1);
  }

  struct server_state current_state;
  reset_state(&current_state);
  int transfer_success = 0;
  while (1)
  {
    clilen = sizeof(cli_addr);
    printf("Waiting...\n");
    bytes_read = recvfrom(sockfd, temp_buffer, uBUFFER_SIZE, 0, (struct sockaddr *)&cli_addr, &clilen);

    if (bytes_read < 0) 
    {
      fprintf(stderr, "ERROR! %d bytes read.\n", bytes_read);
    }

    current_state.cli_addr = &cli_addr;
    current_state.clilen = clilen;
    current_state.sockfd = sockfd;

    //Set the timeout value! Get reaaaady!
    struct timeval timeout_val;
    timeout_val.tv_sec = uTIMEOUT_SEC;  // 0 full seconds timeout
    timeout_val.tv_usec = uTIMEOUT_US;  // But actually 3000 us == 3 ms
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout_val,sizeof(struct timeval));

    printf("-------------------------------------------------------\n");
    transfer_success = begin_transfer(temp_buffer, bytes_read, &current_state);
    fprintf(stderr, "Transfer returned %d. Resetting state and timeout\n", transfer_success);
    reset_state(&current_state);

    //Turn the timeout value off
    timeout_val.tv_sec = 0;  // no timeout
    timeout_val.tv_usec = 0;  //
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout_val,sizeof(struct timeval));
  }
}  


/*
                                                         
                                                                       
 ooooo  o 8          ooooo                            d'b              
 8        8            8                              8                
o8oo   o8 8 .oPYo.     8   oPYo. .oPYo. odYo. .oPYo. o8P  .oPYo. oPYo. 
 8      8 8 8oooo8     8   8  `' .oooo8 8' `8 Yb..    8   8oooo8 8  `' 
 8      8 8 8.         8   8     8    8 8   8   'Yb.  8   8.     8     
 8      8 8 `Yooo'     8   8     `YooP8 8   8 `YooP'  8   `Yooo' 8     
:..:::::....:.....:::::..::..:::::.....:..::..:.....::..:::.....:..::::
:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::://////////
*/
// Constants and things
#define uMAX_TIMEOUTS 5
#define uMAX_SEQ_NUM 50

//FORWARD DECLARATIONS
int validate_and_unpack (char* pmessage_buffer, int imessage_size, int iexpected_type, void* pdestination_message);
int transfer_file (struct server_state* pcurrent_state);
int send_data_msg(char* pdata_buffer, int size_of_data, int seq_no, struct server_state* pcurrent_state);
int send_error_msg(struct server_state* pcurrent_state);
int send_window(struct server_state* pcurrent_state);
int wait_for_ACK(struct server_state* pcurrent_state);

//Takes in the first message read, a file descriptor for the outbound UDP socket, 
//  and the client's address. Parses the message, routes to appropriate 
//  action
int begin_transfer(char* pmessage_buffer, int imessage_size, struct server_state* pcurrent_state) 
{
  struct message_1RRQ RRQ_msg_unpacked;
  if (validate_and_unpack(pmessage_buffer, imessage_size, MESSAGE_TYPE_RRQ, &RRQ_msg_unpacked) < 0) {
    fprintf(stderr, "ERROR unpacking! Exiting!\n");
    return -1;
  }
  // otherwise, RRQ_msg_unpacked is good to use!
  strcpy (pcurrent_state->filename, RRQ_msg_unpacked.filename);
  pcurrent_state->win_size = RRQ_msg_unpacked.win_size;
  fprintf(stderr, "Request %u parsed for filename '%s' with window size %u\n", RRQ_msg_unpacked.msg_type,
                                                                            pcurrent_state->filename,
                                                                            pcurrent_state->win_size);
  // state is initialized, begin sending for real!
  transfer_file(pcurrent_state);


  return 1;
}


int transfer_file (struct server_state* pcurrent_state)
{
  fprintf(stderr, "Transferring file %s.\n", pcurrent_state->filename);
  FILE* pfile_to_transfer = fopen(pcurrent_state->filename, "rb");
  if (!pfile_to_transfer)
  {
    fprintf(stderr, "ERROR opening file %s\n", pcurrent_state->filename);
    send_error_msg(pcurrent_state);
    return -1;
  }

  //Get the size of the file and calculate the number of sequences and the size
  //of the last one
  fseek(pfile_to_transfer, 0L, SEEK_END);
  pcurrent_state->file_size = ftell(pfile_to_transfer);
  //Reset back to beginning
  fseek(pfile_to_transfer, 0L, SEEK_SET);
  fprintf(stderr, "File size is %u bytes.\n", pcurrent_state->file_size);

  pcurrent_state->max_seq_no = pcurrent_state->file_size / uDATASIZE; 
  pcurrent_state->final_seq_size = pcurrent_state->file_size % uDATASIZE;
  fprintf(stderr, "Max seq number is %u. Final seq size is %u.\n", pcurrent_state->max_seq_no, 
                                                                  pcurrent_state->final_seq_size);
  if (pcurrent_state->win_size > pcurrent_state->max_seq_no + 1) 
  {
    pcurrent_state->win_size = pcurrent_state->max_seq_no + 1;
    fprintf(stderr, "Specified impossible window size. Making max %u instead.\n", pcurrent_state->win_size);
    
  }

  pcurrent_state->final_window_bottom = pcurrent_state->max_seq_no - pcurrent_state->win_size + 1;
  fprintf(stderr, "Final window bottom is %u.\n", pcurrent_state->final_window_bottom);

  pcurrent_state->pfile_to_transfer = pfile_to_transfer;

  int send_status = 0;

  fprintf(stderr, "Sending first window.\n");
  pcurrent_state->consecutive_timeouts = 0; // make sure this is zero before calling send window
  send_status = send_window(pcurrent_state);
  if (send_status == 1) {
      return 1;
  }
  else {
    fprintf(stderr, "Window %u completely and utterly failed. Cancelling transmission.\n", pcurrent_state->last_seq_sent);
    return -1;
  }

  return -1;

}

int send_window(struct server_state* pcurrent_state)
{
  int window_bottom = pcurrent_state->recent_ACK_received + 1;
  int window_top = window_bottom + pcurrent_state->win_size - 1;
  fprintf(stderr, "Sending window from %u to %u...\n", window_bottom, window_top);

  if (window_top >= pcurrent_state->max_seq_no) 
  { // if it is the final window, set the the flag (this becomes important in wait_for_ACK)
    fprintf(stderr, "Just kidding, this is the final window!\n");
    window_top = pcurrent_state->max_seq_no;
    fprintf(stderr, "Its new bottom - top is %u - %u!\n", window_bottom, window_top);
    if (window_bottom > pcurrent_state->max_seq_no)
    {
      return 1;
    }
    fprintf(stderr, "Setting final window flag!\n");
    pcurrent_state->this_is_final_window = 1;
  }

  //The first seq in this window is the window on the bottom
  int current_seq_loading = pcurrent_state->last_seq_sent + 1;
  int bytes_loaded ;

  int seq_start;

  char current_data_buffer[uDATASIZE];
  char temp_char;

  while (current_seq_loading <= window_top)
  {
    bytes_loaded = 0;
    //load that seq, char by char!
    //seek to its start first!
    seq_start = current_seq_loading * uDATASIZE;
    fseek(pcurrent_state->pfile_to_transfer, seq_start, SEEK_SET);

    if (!pcurrent_state->this_is_final_window || current_seq_loading < pcurrent_state->max_seq_no)
    { //if this isn't the final window, we can't possibly be on the last seq, and
      //don't have to worry about it not being 512
      fprintf(stderr, "Loading and shipping seq number %u from %u to %u\n", current_seq_loading, seq_start, seq_start+uDATASIZE-1);
      while (bytes_loaded < uDATASIZE)
      {
        temp_char = fgetc(pcurrent_state->pfile_to_transfer);
        current_data_buffer[bytes_loaded] = temp_char;
        bytes_loaded++;
      }
      //Bytes are loaded - let's ship this bitch!!
      send_data_msg(current_data_buffer, uDATASIZE, current_seq_loading, pcurrent_state);
    }
    else 
    { //this is the last seq - treat it gingerly!
      fprintf(stderr, "Loading and shipping LAST SEQ EVER %u of size %u, starting from %u\n", current_seq_loading, pcurrent_state->final_seq_size, seq_start);
      while (bytes_loaded < pcurrent_state->final_seq_size)
      {
        temp_char = fgetc(pcurrent_state->pfile_to_transfer);
        current_data_buffer[bytes_loaded] = temp_char;
        bytes_loaded++;
      }
      //Bytes are loaded - we outtie!!
      send_data_msg(current_data_buffer, bytes_loaded, current_seq_loading, pcurrent_state);
    }
    current_seq_loading++;
  }
  
  pcurrent_state->last_seq_sent = current_seq_loading-1;

  fprintf(stderr, "Waiting for ACKs, starting with last seq %u\n", pcurrent_state->last_seq_sent);
  // window has been sent! now we wait for acks!
  return wait_for_ACK(pcurrent_state);

}


int wait_for_ACK(struct server_state* pcurrent_state)
{

  int bytes_read;
  char baby_buffer[uBABY_BUFFER];
  bytes_read = recvfrom(pcurrent_state->sockfd, baby_buffer, uBABY_BUFFER, 0, 
                                        (struct sockaddr *)pcurrent_state->cli_addr, &(pcurrent_state->clilen));
  if (bytes_read <= 0) 
  { // musta been a timeout! let's send it all again
    fprintf(stderr, "Timeout number %d on ack %d. Resending?\n", pcurrent_state->consecutive_timeouts, 
                                                                    pcurrent_state->recent_ACK_received+1);
    pcurrent_state->consecutive_timeouts++; // add one to the count!
    if (pcurrent_state->consecutive_timeouts >= uMAX_TIMEOUTS) 
    { // if we've already hit 5
      fprintf(stderr, "Limit of %d timeouts reached. Reporting failure.\n", uMAX_TIMEOUTS);
      return -1; // we ran out =[ ... let the program know
    }
    // we didn't hit 5, so resend
    fprintf(stderr, "Resending! Setting last_seq_sent %d to the last ACK received %d\n", pcurrent_state->last_seq_sent, pcurrent_state->recent_ACK_received);
    pcurrent_state->last_seq_sent = pcurrent_state->recent_ACK_received;
    return send_window(pcurrent_state);
  }
  else 
  { // we got something! was it an ACK?! stay tuned and we'll find out!
    struct message_3ACK current_ACK_msg;
    if (validate_and_unpack(baby_buffer, bytes_read, MESSAGE_TYPE_ACK, &current_ACK_msg) < 1) {return -1;};
    fprintf(stderr, "Received ack for seq_no %u\n", current_ACK_msg.seq_no);

    if (current_ACK_msg.seq_no >= pcurrent_state->recent_ACK_received)
    { // if it's greater than any ACK we've gotten before
      pcurrent_state->consecutive_timeouts = 0; //reset the count!
      if (pcurrent_state->this_is_final_window)
      {      // if this is the last window
        if (current_ACK_msg.seq_no == pcurrent_state->max_seq_no)
        { // if it's the last seq ever, we're done!
          fprintf(stderr, "Received final ACK number %u! Transfer complete!\n", pcurrent_state->max_seq_no);
          return 1;
        }
        else
        { // just shift the final window over a crack
          pcurrent_state->recent_ACK_received = current_ACK_msg.seq_no;
          return wait_for_ACK(pcurrent_state);
        }
      }
        // if this isn't the final window
        // shift the window and call send_window      
        pcurrent_state->recent_ACK_received = current_ACK_msg.seq_no;
        fprintf(stderr, "Got ACK %u...Shifting window!\n", pcurrent_state->recent_ACK_received);
        return send_window(pcurrent_state);
      }
    else // if it's out of order (damn UDP messing things up)
    {
      return wait_for_ACK(pcurrent_state);
    }

  }
  return -1;
}


int send_data_msg(char* pdata_buffer, int size_of_data, int seq_no, struct server_state* pcurrent_state)
{
  struct message_2DATA data_message;
  memcpy(data_message.data, pdata_buffer, size_of_data);
  data_message.msg_type = MESSAGE_TYPE_DATA;
  data_message.seq_no = seq_no;

  //fprintf(stderr, "%s\n", data_message.data);

  char* msg_as_pointer = (char*) &data_message;

  int bytes_sent;
  bytes_sent = sendto(pcurrent_state->sockfd, msg_as_pointer, size_of_data+2, 0, (struct sockaddr *)pcurrent_state->cli_addr, pcurrent_state->clilen);
  if (bytes_sent <= 0)
  {
    return -1;
  }
  return 1;
}

int send_error_msg(struct server_state* pcurrent_state)
{
  fprintf(stderr, "Sending ERROR message.\n");
  struct message_4ERROR error_message;
  error_message.msg_type = MESSAGE_TYPE_ERROR;

  char* msg_as_pointer = (char*) &error_message;
  
  int bytes_sent;
  bytes_sent = sendto(pcurrent_state->sockfd, msg_as_pointer, sizeof(error_message), 0, (struct sockaddr *)pcurrent_state->cli_addr, pcurrent_state->clilen);
  if (bytes_sent <= 0)
  {
    return -1;
  }
  return 1;
}

int validate_and_unpack (char* pmessage_buffer, int imessage_size, int iexpected_type, void* pdestination_message) 
{
  
  switch (iexpected_type){
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
      fprintf(stderr, "ERROR: server received DATA message? wut?\n");
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
      fprintf(stderr, "Grabbed meessage type %u\n", pACK_msg_unpacked->msg_type);
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
  }

  return -1;



}

