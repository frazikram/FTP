/* 
 * server FTP program
 *
 * NOTE: Starting homework #2, add more comments here describing the overall function
 * performed by server ftp program
 * This includes, the list of ftp commands processed by server ftp.
 *
 */

#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#define SERVER_FTP_PORT 5236
#define DATA_CONNECTION_PORT  5237


/* Error and OK codes */
#define OK 0
#define ER_INVALID_HOST_NAME -1
#define ER_CREATE_SOCKET_FAILED -2
#define ER_BIND_FAILED -3
#define ER_CONNECT_FAILED -4
#define ER_SEND_FAILED -5
#define ER_RECEIVE_FAILED -6
#define ER_ACCEPT_FAILED -7

/* for counting words */

#define OUT    0
#define IN    1

/*Debug */
#define DEBUG 0

/* Function prototypes */

int clntConnect(char	*serverName, int *s);
int svcInitServer(int *s);
int sendMessage (int s, char *msg, int  msgSize);
int receiveMessage(int s, char *buffer, int  bufferSize, int *msgSize);
unsigned countWords(char *str);
int whichcmd(char *x, char *y, int count);
void CopyResulttxt(char * MessagetoSend);
//My functions prototype here.

/* List of all global variables */

char userCmd[1024];	/* user typed ftp command line received from client */
char cmd[1024];		/* ftp command (without argument) extracted from userCmd */
char argument[1024];	/* argument (without ftp command) extracted from userCmd */
char replyMsg[1024];       /* buffer to send reply message to client */

char sendBuff[100];
char recvBuff[100];

char swd[1024]; /* Server root directory */
char resultstring[1024]; /* Complete path to the result.txt */
char systemstring[1024]; /* Complete path to the result.txt */
/*
 * main
 *
 * Function to listen for connection request from client
 * Receive ftp command one at a time from client
 * Process received command
 * Send a reply message to the client after processing the command with staus of
 * performing (completing) the command
 * On receiving QUIT ftp command, send reply to client and then close all sockets
 *
 * Parameters
 * argc		- Count of number of arguments passed to main (input)
 * argv  	- Array of pointer to input parameters to main (input)
 *		   It is not required to pass any parameter to main
 *		   Can use it if needed.
 *
 * Return status
 *	0			- Successful execution until QUIT command from client 
 *	ER_ACCEPT_FAILED	- Accepting client connection request failed
 *	N			- Failed stauts, value of N depends on the command processed
 */

int main(	
         int argc,
         char *argv[]
                )
{
  /* List of local varibale */

  int msgSize;        /* Size of msg received in octets (bytes) */
  int listenSocket;   /* listening server ftp socket for client connect request */
  int ccSocket;        /* Control connection socket - to be used in all client communication */
  int status;


  if (getcwd(swd, sizeof(swd)) != NULL)
    fprintf(stdout, "Server root dir: %s\n", swd);
  //Building outputs string for system calls.
  //strcpy(resultstring, " > ");
  strcat(resultstring, swd);
  strcat(resultstring, "/result.txt");


  /*
   * NOTE: without \n at the end of format string in printf,
   * UNIX will buffer (not flush)
   * output to display and you will not see it on monitor.
   */
  printf("Started execution of server ftp\n");


  /*initialize server ftp*/
  printf("Initialize ftp server\n");	/* changed text */

  status=svcInitServer(&listenSocket);
  if(status != 0)
    {
      printf("Exiting server ftp due to svcInitServer returned error\n");
      exit(status);
    }


  printf("ftp server is waiting to accept connection\n");

  /* wait until connection request comes from client ftp */
  ccSocket = accept(listenSocket, NULL, NULL);

  printf("Came out of accept() function \n");

  if(ccSocket < 0)
    {
      perror("cannot accept connection:");
      printf("Server ftp is terminating after closing listen socket.\n");
      close(listenSocket);  /* close listen socket */
      return (ER_ACCEPT_FAILED);  // error exist
    }

  printf("Connected to client, calling receiveMsg to get ftp cmd from client \n");


  /* Receive and process ftp commands from client until quit command.
   * On receiving quit command, send reply to client and 
   * then close the control connection socket "ccSocket". 
   */
  do
    {

      memset(userCmd,'\0',sizeof(userCmd));
      memset(cmd,'\0',sizeof(cmd));
      memset(argument,'\0',sizeof(argument));
      memset(replyMsg,'\0',sizeof(replyMsg));

      /* Receive client ftp commands until */
      status=receiveMessage(ccSocket, userCmd, sizeof(userCmd), &msgSize);
      if(status < 0)
        {
          printf("Receive message failed. Closing control connection \n");
          printf("Server ftp is terminating.\n");
          break;
        }
      int count = countWords(userCmd);
      if (count == 0) {
        continue;
      }
      else if (count == 1)
        {
          strcpy(cmd,userCmd);
        }
      else if (count == 2) {
        strcpy(cmd, strtok(userCmd, " "));
        strcpy(argument, strtok(NULL, " "));
        strtok(argument, "\n");/*Get rid of \n */
      }

      if(whichcmd(cmd, argument,count)){

        /*
         * ftp server sends only one reply message to the client for 
         * each command received in this implementation.
         */
        /* In our case, we will send back result.txt" */
        /* Read result.txt in a string and send it back */
        /* strcpy(replyMsg,": 200 cmd okay\n");  /* Should have appropriate reply msg starting HW2 */
        CopyResulttxt(replyMsg);
      } else {
        /* we need to send error. */
        strcat(replyMsg, ": Error. Command execution failure");
      }


      status=sendMessage(ccSocket,replyMsg,strlen(replyMsg) + 1);
      /* Delete the result.txt if it exists*/
            
      /*  if( access( "result.txt", F_OK ) != -1 ) */
      struct stat buffer;
      int exist = stat(resultstring,&buffer);
      if(exist == 0){
        strcpy(systemstring, "rm ");
        strcat(systemstring, resultstring);
        system(systemstring);
        //system("rm result.txt");

      }
      /*  printf("Error: Cannot remove result.txt\n"); */
            
      /* the reply string strlen does not count NULL character */
      if(status < 0)
        {
          break;  /* exit while loop */
        }
    }
  while(strcmp(cmd, "quit") != 0);

  printf("Closing control connection socket.\n");
  close (ccSocket);  /* Close client control connection socket */

  printf("Closing listen socket.\n");
  close(listenSocket);  /*close listen socket */

  printf("Existing from server ftp main \n");

  return (status);
}


/*
 * svcInitServer
 *
 * Function to create a socket and to listen for connection request from client
 *    using the created listen socket.
 *
 * Parameters
 * s		- Socket to listen for connection request (output)
 *
 * Return status
 *	OK			- Successfully created listen socket and listening
 *	ER_CREATE_SOCKET_FAILED	- socket creation failed
 */

int svcInitServer (
                   int *s 		/*Listen socket number returned from this function */
                   )
{
  int sock;
  struct sockaddr_in svcAddr;
  int qlen;

  /*create a socket endpoint */
  if( (sock=socket(AF_INET, SOCK_STREAM,0)) <0)
    {
      perror("cannot create socket");
      return(ER_CREATE_SOCKET_FAILED);
    }

  /*initialize memory of svcAddr structure to zero. */
  memset((char *)&svcAddr,0, sizeof(svcAddr));

  /* initialize svcAddr to have server IP address and server listen port#. */
  svcAddr.sin_family = AF_INET;
  svcAddr.sin_addr.s_addr=htonl(INADDR_ANY);  /* server IP address */
  svcAddr.sin_port=htons(SERVER_FTP_PORT);    /* server listen port # */

  /* bind (associate) the listen socket number with server IP and port#.
   * bind is a socket interface function 
   */
  if(bind(sock,(struct sockaddr *)&svcAddr,sizeof(svcAddr))<0)
    {
      perror("cannot bind");
      close(sock);
      return(ER_BIND_FAILED);	/* bind failed */
    }

  /* 
   * Set listen queue length to 1 outstanding connection request.
   * This allows 1 outstanding connect request from client to wait
   * while processing current connection request, which takes time.
   * It prevents connection request to fail and client to think server is down
   * when in fact server is running and busy processing connection request.
   */
  qlen=1; 


  /* 
   * Listen for connection request to come from client ftp.
   * This is a non-blocking socket interface function call, 
   * meaning, server ftp execution does not block by the 'listen' funcgtion call.
   * Call returns right away so that server can do whatever it wants.
   * The TCP transport layer will continuously listen for request and
   * accept it on behalf of server ftp when the connection requests comes.
   */

  listen(sock,qlen);  /* socket interface function call */

  /* Store listen socket number to be returned in output parameter 's' */
  *s=sock;

  return(OK); /*successful return */
}


/*
 * sendMessage
 *
 * Function to send specified number of octet (bytes) to client ftp
 *
 * Parameters
 * s		- Socket to be used to send msg to client (input)
 * msg  	- Pointer to character arrary containing msg to be sent (input)
 * msgSize	- Number of bytes, including NULL, in the msg to be sent to client (input)
 *
 * Return status
 *	OK		- Msg successfully sent
 *	ER_SEND_FAILED	- Sending msg failed
 */

int sendMessage(
                int    s,	/* socket to be used to send msg to client */
                char   *msg, 	/* buffer having the message data */
                int    msgSize 	/* size of the message/data in bytes */
                )
{
  int i;


  /* Print the message to be sent byte by byte as character */
  for(i=0; i<msgSize; i++)
    {
      printf("%c",msg[i]);
    }
  printf("\n");

  if((send(s, msg, msgSize, 0)) < 0) /* socket interface call to transmit */
    {
      perror("unable to send ");
      return(ER_SEND_FAILED);
    }

  return(OK); /* successful send */
}


/*
 * receiveMessage
 *
 * Function to receive message from client ftp
 *
 * Parameters
 * s		- Socket to be used to receive msg from client (input)
 * buffer  	- Pointer to character arrary to store received msg (input/output)
 * bufferSize	- Maximum size of the array, "buffer" in octent/byte (input)
 *		    This is the maximum number of bytes that will be stored in buffer
 * msgSize	- Actual # of bytes received and stored in buffer in octet/byes (output)
 *
 * Return status
 *	OK			- Msg successfully received
 *	R_RECEIVE_FAILED	- Receiving msg failed
 */


int receiveMessage (
                    int s, 		/* socket */
                    char *buffer, 	/* buffer to store received msg */
                    int bufferSize, /* how large the buffer is in octet */
                    int *msgSize 	/* size of the received msg in octet */
                    )
{
  int i;

  *msgSize=recv(s,buffer,bufferSize,0); /* socket interface call to receive msg */

  if(*msgSize<0)
    {
      perror("unable to receive");
      return(ER_RECEIVE_FAILED);
    }

  /* Print the received msg byte by byte */
  for(i=0;i<*msgSize;i++)
    {
      printf("%c", buffer[i]);
    }
  printf("\n");

  return (OK);
}

unsigned countWords(char *str)
{
  int state = OUT;
  unsigned wc = 0;  // word count

  // Scan all characters one by one
  while (*str)
    {
      // If next character is a separator, set the 
      // state as OUT
      if (*str == ' ' || *str == '\n' || *str == '\t')
        state = OUT;

      // If next character is not a word separator and 
      // state is OUT, then set the state as IN and 
      // increment word count
      else if (state == OUT)
        {
          state = IN;
          ++wc;
        }

      // Move to next character
      ++str;
    }

  return wc;
}

/* We will use system call to execute the command if it is legal.*/
/* The output of the system command will be stored in result.txt*/
int whichcmd(char *cmd, char *argument, int count)
{
  if (count == 1) {

    int check = !strcmp(cmd, "pwd") || !strcmp(cmd, "ls") || !strcmp(cmd, "status") || !strcmp(cmd, "stat") || !strcmp(cmd, "stat") || !strcmp(cmd, "help") || !strcmp(cmd, "quit");
    if(check == 1){

      if (strcmp(cmd, "pwd") == 0)
        {
          if(DEBUG)
            printf("Executing pwd command\n");
          
          strcpy(systemstring, "pwd > ");
          strcat(systemstring, resultstring);
          int status = system(systemstring);
          // int status = system("pwd > result.txt");
          if(status == 0)
            return 1;
          else 
            return 0;

        }
      else if (strcmp(cmd, "ls") == 0)
        {
          if(DEBUG)
            printf("Executing ls command\n");
          
          strcpy(systemstring, "ls > ");
          strcat(systemstring, resultstring);
          int status = system(systemstring);
          // int status = system("ls > result.txt");
          if(status == 0)
            return 1;
          else 
            return 0;

        }
      else if ((strcmp(cmd, "stat") == 0)|| (strcmp(cmd, "status") == 0))
        {
          if(DEBUG)
            printf("Executing stat command");
          strcpy(systemstring, "echo \"Status: OK\n\" >  ");
          strcat(systemstring, resultstring);
          int status = system(systemstring);

          // int status = system("echo \"Status: OK\n\" > result.txt");
          if(status == 0)
            return 1;
          else 
            return 0;


        }
      else if (strcmp(cmd, "help") == 0)
        {
          if(DEBUG)
            printf("Executing help command");
          strcpy(systemstring, " echo \"Executed  help command.\n\" >  ");
          strcat(systemstring, resultstring);
          int status = system(systemstring);

          //int status = system("echo \"Executed  help command.\n\" > result.txt");
          if(status == 0)
            return 1;
          else 
            return 0;
        }
      else if (strcmp(cmd, "quit") == 0)
        {
          if(DEBUG)
            printf("Executing quit command");
          strcpy(systemstring, " echo \"Executed quit command. \n\" >  ");
          strcat(systemstring, resultstring);
          int status = system(systemstring);
          //int status = system("echo \"Executed quit command. \n\" > result.txt");
          if(status == 0)
            return 1;
          else 
            return 0;
        }
    } else {
      return 0;
    }
  }
  else  if (count == 2) {

    int check = !strcmp(cmd, "user") || !strcmp(cmd, "pass") || !strcmp(cmd, "quit") || !strcmp(cmd, "mkdir") || !strcmp(cmd, "rmdir") || !strcmp(cmd, "cd") || !strcmp(cmd, "dele")|| !strcmp(cmd, "send") || !strcmp(cmd, "recv")|| !strcmp(cmd, "put") || !strcmp(cmd, "get");
	
    if (check == 1) {

      if (strcmp(cmd, "user") == 0) 
        {
          if(DEBUG)
            printf("Executing user command");
          strcpy(cmd, "id ");
          strcat(cmd, argument);
          strcat(cmd, " > ");
          strcat(cmd, resultstring);
          int status = system(cmd);
          if(status == 0)
            return 1;
          else 
            return 0;
          
        }
      else if (strcmp(cmd, "pass") == 0)
        {
          if(DEBUG)
            printf("Executing pass command");
          strcpy(systemstring, "echo \"Dummy: passwd  OK\n\"  >  ");
          strcat(systemstring, resultstring);
          int status = system(systemstring);


          //          int status = system("echo \"Dummy: passwd  OK\n\" > result.txt");
          if(status == 0)
            return 1;
          else 
            return 0;

        }
      else if (strcmp(cmd, "mkdir") == 0)
        {
          if(DEBUG)
            printf("Executing mkdir command");
          
          if( access( argument, F_OK ) != -1 ){

          strcpy(systemstring, "echo \"Unable to create the directory. It already exists.\n\"  >  ");
          strcat(systemstring, resultstring);
          system(systemstring);
           return 1;

          }

          strcpy(cmd, "mkdir ");
          strcat(cmd, argument);
          int status = system(cmd);
          if(status == 0) {
          strcpy(systemstring, "echo \"directory created.\n\"  >  ");
          strcat(systemstring, resultstring);
          system(systemstring);

          //  system("echo \"directory created.\n\" > result.txt");
            return 1;
          } else {
          strcpy(systemstring, "echo \"Unable to create the directory.\n\"  >  ");
          strcat(systemstring, resultstring);
          system(systemstring);
           return 1;
          }
        }
      else if (strcmp(cmd, "rmdir") == 0)
        {
          if(DEBUG)
            printf("Executing rmdir command");

          strcpy(cmd, "rmdir ");
          strcat(cmd, argument);
          int status = system(cmd);
          if(status == 0) {
          strcpy(systemstring, " echo \"directory removed.\n\" >  ");
          strcat(systemstring, resultstring);
          system(systemstring);
          // system("echo \"directory removed.\n\" > result.txt");
            return 1;
          } else {
            strcpy(systemstring, "echo \"Unable to remove the directory.\n\"  >  ");
            strcat(systemstring, resultstring);
            system(systemstring);
            return 1;

          }

        }
      else if (strcmp(cmd, "cd") == 0)
        {
          if(DEBUG)
            printf("Executing cd command");

          int status = chdir(argument);
          if(status == 0) {


          strcpy(systemstring, " echo \"directory changed.\n\" >  ");
          strcat(systemstring, resultstring);
          system(systemstring);
          //system("echo \"directory changed.\n > result.txt");            
            return 1;
          } else {

            strcpy(systemstring, "echo \"Unable to change the directory.\n\"  >  ");
            strcat(systemstring, resultstring);
            system(systemstring);


            return 1;    

          }
        }
      else if (strcmp(cmd, "dele") == 0)
        {
          if(DEBUG)
            printf("Executing dele command");
          strcpy(cmd, "rm ");
          strcat(cmd, argument);
          int status = system(cmd);
          if(status == 0) {

          strcpy(systemstring, " echo \"file removed.\n\" >  ");
          strcat(systemstring, resultstring);
          system(systemstring);

          //system("echo \"file removed.\n\" > result.txt");
            return 1;
          } else {
            strcpy(systemstring, "echo \"Unable remove the file.\n\"  >  ");
            strcat(systemstring, resultstring);
            system(systemstring);
            return 1;   
          }
        }
      else if ((strcmp(cmd, "recv") == 0)||(strcmp(cmd, "get") == 0))
        {
          if(DEBUG)
            printf("Executing recv/get command");

          /* Client has opened a data transfer socket. */
          /*  Server needs to connect with it using clntConnect*/
          int ccSocket=0;
          int status =clntConnect("127.0.0.1", &ccSocket);
          /*0 return status mean successs*/
          if(status != 0)
            {
              printf("Connection to client failed, exiting.. \n");

              strcpy(systemstring, "echo \"Connection to client failed.\n\"  >  ");
              strcat(systemstring, resultstring);
              system(systemstring);
              //system("echo \"Connection to client failed.\n\" > result.txt");
              return 0;
            }
          else {
            /* Open the file to transfer */
            /* The file should exist and */
            /*Server should have read permission to it */
            if( access( argument, R_OK ) == -1 ) {
              /*File doest not exist or does not have read permission. */
              printf("File %s doest not exist or does not have read permission. \n",argument);
              strcpy(systemstring, "echo \"File doest not exist or does not have read permission.\n\"  >  ");
              strcat(systemstring, resultstring);
              system(systemstring);

              //system("echo \"File doest not exist or does not have read permission.\n\" > result.txt");
              close(ccSocket);   
              return 0;
            } else {
              /*File does exist. Open to read.*/
              printf("Sending file %s \n",argument);

              mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
              int fd = open(argument, O_RDONLY,mode);
              
              if (fd == -1) {
                printf("Unknown error in opening the file.\n");
              }
              while(1) {
            
                // Read data into buffer.  
                // We have limited buffer size.
                // File may be bugger or smaller than buffer.
                // We may not have enough to fill up buffer, so we
                // store how many bytes were actually read in bytes_read.
                // We may not have enough buffer to load file.
                // In that case, we will iterate through the file using read.
                int bytes_read = read(fd, sendBuff, sizeof(sendBuff));
                if (bytes_read == 0) // We're done reading from the file
                  break;

                if (bytes_read < 0) {
                  printf("Unknown error in reading the file.\n");
                  break;
                }
                //Handling congestion.
                // You need a loop for the write, because not all of the data may be written
                // in one call; write will return how many bytes were written. p keeps
                // track of where in the buffer we are, while we decrement bytes_read
                // to keep track of how many bytes are left to write.
                void *p = sendBuff;
                while (bytes_read > 0) {
                  int bytes_written = write(ccSocket, p, bytes_read);

                  /*We can retry to send few times*/

                  if (bytes_written <= 0) {
                    printf("Unknown error in sending the file.\n");
                    // handle errors
                  }

                  bytes_read = bytes_read - bytes_written;
                  p = p +  bytes_written;
 
                }
              }/*while(1)*/

              close(fd);

              close(ccSocket);  
              strcpy(systemstring, "echo \"File sent.\n\"  >  ");
              strcat(systemstring, resultstring);
              system(systemstring);
              //system("echo \"file sent.\n\" > result.txt");
              return 1; 
            }

          }

        
        }
      else if ((strcmp(cmd, "send") == 0)||(strcmp(cmd, "put") == 0))
        {
          if(DEBUG)
            printf("Executing recv/get command");

          /* Client has opened a data transfer socket. */
          /*  Server needs to connect with it using clntConnect*/
          int ccSocket=0;
          int status =clntConnect("127.0.0.1", &ccSocket);
          /*0 return status mean successs*/
          if(status != 0)
            {
              printf("Connection to client failed, exiting.. \n");

              strcpy(systemstring, "echo \"Connection to client failed.\n\"  >  ");
              strcat(systemstring, resultstring);
              system(systemstring);
              //              system("echo \"Connection to client failed.\n\" > result.txt");
              return 0;
            }
          else {
            printf("Ready to get the file. \n");
            /*open the file to write */
            /*If a file exists already, We quit because we dont want to overwrite it. */
            /*Check the existence of the file */
            if( access( argument, F_OK ) != -1 ) {
              /*File exists*/
              printf("File %s already exist. cannot overwrite it. \n",argument);

              strcpy(systemstring, "echo \"Error: File already exist. cannot overwrite it.\n\"  >  ");
              strcat(systemstring, resultstring);
              system(systemstring);
              //system("echo \"Error: File already exist. cannot overwrite it. \n\" > result.txt");
              close(ccSocket); 
              return 1;
            } else {
              /*File does not exist. Open to write.*/
              printf("Writing file %s \n",argument);
              FILE* fd = fopen(argument, "wr");
              
              if (fd == NULL) {
                printf("Unknown error in opening the file.\n");
                return 0;
              }
              while(1) {
            
                // Read data into buffer.  
                // We have limited buffer size.
                // File may be bugger or smaller than buffer.
                // We may not have enough to fill up buffer, so we
                // store how many bytes were actually read in bytes_read.
                // We may not have enough buffer to load file.
                // In that case, we will iterate through the file using read.
                memset(recvBuff,'\0',sizeof(recvBuff)); /* Cleanup everytime */
                int bytes_read = read(ccSocket, recvBuff, sizeof(recvBuff));
                if (bytes_read == 0) // We're done reading from the socket
                  break;

                if (bytes_read < 0) {
                  printf("Unknown error in reading the socket.\n");
                  break;
                }
                //Handling congestion.
                // You need a loop for the write, because not all of the data may be written
                // in one call; write will return how many bytes were written. p keeps
                // track of where in the buffer we are, while we decrement bytes_read
                // to keep track of how many bytes are left to write.
                void *p = recvBuff;

                int bytes_written = fputs(p,fd);

                /*We can retry to send few times*/

                if (bytes_written <= 0) {
                  printf("Unknown error in writing the file.\n");
                  // handle errors
                }
              }/*while(1)*/

              fclose(fd);

              close(ccSocket);   
              close(ccSocket);  
              strcpy(systemstring, "echo \"File received.\n\"  >  ");
              strcat(systemstring, resultstring);
              system(systemstring);
              // system("echo \"file received.\n\" > result.txt");
              return 1;
    
            }
          }        
        }

    }else {
      return 0;
    }
  }
  else {

    printf("Error. Something went wrong here\n");
    return 1;
  }
}

/* Read result.txt into a buffer and copy that buffer to Message to send */
void CopyResulttxt(char * MessagetoSend) {

  char * buffer = 0;
  long length;
  FILE * f = fopen("result.txt","rb");

  if (f)
    {
      fseek (f, 0, SEEK_END);
      length = ftell (f);
      fseek (f, 0, SEEK_SET);
      buffer = malloc (length);
      if (buffer)
        {
          fread (buffer, 1, length, f);
        }
      fclose (f);
    }
  else {
    printf("Error: Cannot open result.txt\n");
  }

  if (buffer)
    {
      strcpy(MessagetoSend,buffer);
    }
}

/*
 * clntConnect
 *
 * Function to create a socket, bind local client IP address and port to the socket
 * and connect to the server
 *
 * Parameters
 * serverName	- IP address of server in dot notation (input)
 * s		- Control connection socket number (output)
 *
 * Return status
 *	OK			- Successfully connected to the server
 *	ER_INVALID_HOST_NAME	- Invalid server name
 *	ER_CREATE_SOCKET_FAILED	- Cannot create socket
 *	ER_BIND_FAILED		- bind failed
 *	ER_CONNECT_FAILED	- connect failed
 */


int clntConnect (
                 char *serverName, /* server IP address in dot notation (input) */
                 int *s 		  /* control connection socket number (output) */
                 )
{
  int sock;	/* local variable to keep socket number */

  struct sockaddr_in clientAddress;  	/* local client IP address */
  struct sockaddr_in serverAddress;	/* server IP address */
  struct hostent	   *serverIPstructure;	/* host entry having server IP address in binary */


  /* Get IP address os server in binary from server name (IP in dot natation) */
  if((serverIPstructure = gethostbyname(serverName)) == NULL)
    {
      printf("%s is unknown server. \n", serverName);
      return (ER_INVALID_HOST_NAME);  /* error return */
    }

  /* Create control connection socket */
  if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
      perror("cannot create socket ");
      return (ER_CREATE_SOCKET_FAILED);	/* error return */
    }

  /* initialize client address structure memory to zero */
  memset((char *) &clientAddress, 0, sizeof(clientAddress));

  /* Set local client IP address, and port in the address structure */
  clientAddress.sin_family = AF_INET;	/* Internet protocol family */
  clientAddress.sin_addr.s_addr = htonl(INADDR_ANY);  /* INADDR_ANY is 0, which means */
  /* let the system fill client IP address */
  clientAddress.sin_port = 0;  /* With port set to 0, system will allocate a free port */
  /* from 1024 to (64K -1) */

  /* Associate the socket with local client IP address and port */
  if(bind(sock,(struct sockaddr *)&clientAddress,sizeof(clientAddress))<0)
    {
      perror("cannot bind");
      close(sock);
      return(ER_BIND_FAILED);	/* bind failed */
    }


  /* Initialize serverAddress memory to 0 */
  memset((char *) &serverAddress, 0, sizeof(serverAddress));

  /* Set ftp server ftp address in serverAddress */
  serverAddress.sin_family = AF_INET;
  memcpy((char *) &serverAddress.sin_addr, serverIPstructure->h_addr, 
         serverIPstructure->h_length);
  serverAddress.sin_port = htons(DATA_CONNECTION_PORT);

  /* Connect to the server */
  if (connect(sock, (struct sockaddr *) &serverAddress, sizeof(serverAddress)) < 0)
    {
      perror("Cannot connect to server ");
      close (sock); 	/* close the control connection socket */
      return(ER_CONNECT_FAILED);  	/* error return */
    }


  /* Store listen socket number to be returned in output parameter 's' */
  *s=sock;

  return(OK); /* successful return */
}  // end of clntConnect() */
