/* 
 * Client FTP program
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

/* Cor counting words */

#define OUT    0
#define IN    1

#define DEBUG 1
/* Function prototypes */

int clntConnect(char	*serverName, int *s);
int sendMessage (int s, char *msg, int  msgSize);
int receiveMessage(int s, char *buffer, int  bufferSize, int *msgSize);
/*Homework 3 , for data transfer */
int svcInitServer(int *s);
//New Function Prototypes
void help(void);
int Notlegalcmd(char *x, char *y, int count);
int dataTransfer(char *x, char *y, char *z);
unsigned countWords(char *str);
/* List of all global variables */

char userCmd[1024];	/* user typed ftp command line read from keyboard */
char cmd[1024];		/* ftp command extracted from userCmd */
char argument[1024];	/* argument extracted from userCmd */
char replyMsg[1024];    /* buffer to receive reply message from server */

char sendBuff[100];
char recvBuff[100];

/*
 * main
 *
 * Function connects to the ftp server using clntConnect function.
 * Reads one ftp command in one line from the keyboard into userCmd array.
 * Sends the user command to the server.
 * Receive reply message from the server.
 * On receiving reply to QUIT ftp command from the server,
 * close the control connection socket and exit from main
 *
 * Parameters
 * argc		- Count of number of arguments passed to main (input)
 * argv  	- Array of pointer to input parameters to main (input)
 *		   It is not required to pass any parameter to main
 *		   Can use it if needed.
 *
 * Return status
 *	OK	- Successful execution until QUIT command from client 
 *	N	- Failed status, value of N depends on the function called or cmd processed
 */

int main(	
	int argc,
	char *argv[]
	)
{
	/* List of local varibale */

	int ccSocket;	/* Control connection socket - to be used in all client communication */
	int msgSize;	/* size of the reply message received from the server */
	int status = OK;

        /*Fraz:Cleanup the strings to be used */

        memset(userCmd,'\0',sizeof(userCmd));
        memset(cmd,'\0',sizeof(cmd));
        memset(argument,'\0',sizeof(argument));
        memset(replyMsg,'\0',sizeof(replyMsg));

	/*
	 * NOTE: without \n at the end of format string in printf,
         * UNIX will buffer (not flush)
	 * output to display and you will not see it on monitor.
 	 */
	printf("Started execution of client ftp\n");


	 /* Connect to client ftp*/
	printf("Calling clntConnect to connect to the server\n");	/* changed text */
        /* Fraz:Comment it out on your school machine.
           status=clntConnect("10.3.200.17", &ccSocket); */
        status=clntConnect("127.0.0.1", &ccSocket);
	/*0 return status mean successs*/
	if(status != 0)
	{
		printf("Connection to server failed, exiting main. \n");
		return (status);
	}


	/* 
	 * Read an ftp command with argument, if any, in one line from user into userCmd.
	 * Copy ftp command part into ftpCmd and the argument into arg array.
 	 * Send the line read (both ftp cmd part and the argument part) in userCmd to server.
	 * Receive reply message from the server.
	 * until quit command is typed by the user.
	 */

	do
          {
            fflush(stdin);
            printf("my ftp> ");

            memset(userCmd,'\0',sizeof(userCmd));/*Clear up userCmd */

            memset(cmd,'\0',sizeof(cmd));/*Clear up ucmd */

            memset(argument,'\0',sizeof(argument));/*Clear up  argument */

            /* read user input and decide to continue if it is nothing */
            /* int status =  read(0,userCmd,128); */
            fgets(userCmd,128,stdin);

            
            int count = countWords(userCmd);
            if(count == 0) {
              continue;
            } else if(count == 1) {                
              strcpy(cmd,strtok(userCmd, "\n"));
              if(!Notlegalcmd (cmd, argument,count)){
                help();
                continue;
              } else {
                /* Check if it is simple help. */
                if(strcmp(cmd,"help") == 0) {
                  help();
                  continue;
                }
                /*Send the cmd as we got only one item to send */
                 status = sendMessage(ccSocket, cmd, strlen(cmd)+1);
              }
                    
            } else if(count == 2) {                
              strcpy(cmd,strtok(userCmd, " "));
              strcpy(argument, strtok(NULL,"\n")); 
              if(!Notlegalcmd (cmd, argument,count)){
                help();
                continue;
              }
              /* Prepare the message to send */

              strcpy(userCmd,cmd);
              strcat(userCmd, " ");
              strcat(userCmd, argument);

              /* Before sending message*/
              /* In case of "send/put", we need to check the existence of file locally. */
              /* If not, print a friendly message locally without bothering server */
              if(strcmp(cmd, "send")== 0 || strcmp(cmd, "put") == 0 ){
                if( access( argument, R_OK ) == -1 ) {
                  /*File doest not exist or does not have read permission. */
                  printf("File %s doest not exist or does not have read permission. \n",argument);
                  continue;
                }
              }
              
              /* In case of "recv/get", we need to check if a file of same name exist locally. */
              /* We wont overwrite the file and print a friendly message locally without bothering server */
              if(strcmp(cmd, "recv") == 0 || strcmp(cmd, "get") == 0 ){
                if( access( argument, F_OK ) != -1 ) {
                  /*File exists*/
                  printf("File %s already exist. Please rename or move it first \n",argument);
                  continue;
                }
              }

              /* send the userCmd to the server */
              status = sendMessage(ccSocket, userCmd, strlen(userCmd)+1);
            
              /* If user command is recv, get, send, put then we need to open a data socket here */
              /* Data socket will be opened after sending the message as client will be stuck on listen socket. */

              if(!strcmp(cmd, "get") || !strcmp(cmd, "recv")){

                if(!dataTransfer(cmd,argument,"write"))
                  continue;

              } else if (!strcmp(cmd, "put") || !strcmp(cmd, "send")){
                if(!dataTransfer(cmd,argument,"read"))
                  continue;
              }


            } else {
              help();
              continue;
            }
                
            /*gets(userCmd);/*Wait for user input, followed by enter*/
            /*strcpy(userCmd, "quit");This statement must be replaced in homework #2 */
            /* to read the command from the user. Use gets or readln function */
		
            /* Separate command and argument from userCmd 
               strcpy(cmd, userCmd);  /* Modify in Homework 2.  Use strtok function 
               strcpy(argument, "");  Modify in Homework 2.  Use strtok function */
           
            if(status != OK)
              {
                break;
              }




            /* Receive reply message from the the server */
            status = receiveMessage(ccSocket, replyMsg, sizeof(replyMsg), &msgSize);
            if(status != OK)
              {
                break;
              }
          }
	while (strcmp(cmd, "quit") != 0);

	printf("Closing control connection \n");
	close(ccSocket);  /* close control connection socket */

	printf("Exiting client main \n");

	return (status);

}  /* end main() */


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
	serverAddress.sin_port = htons(SERVER_FTP_PORT);

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
	int s, 		/* socket to be used to send msg to client */
	char *msg, 	/*buffer having the message data */
	int msgSize 	/*size of the message/data in bytes */
	)
{
	int i;


	/* Print the message to be sent byte by byte as character */
	for(i=0;i<msgSize;i++)
	{
		printf("%c",msg[i]);
	}
	printf("\n");

	if((send(s,msg,msgSize,0)) < 0) /* socket interface call to transmit */
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
 *	ER_RECEIVE_FAILED	- Receiving msg failed
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


/*
 * clntExtractReplyCode
 *
 * Function to extract the three digit reply code 
 * from the server reply message received.
 * It is assumed that the reply message string is of the following format
 *      ddd  text
 * where ddd is the three digit reply code followed by or or more space.
 *
 * Parameters
 *	buffer	  - Pointer to an array containing the reply message (input)
 *	replyCode - reply code number (output)
 *
 * Return status
 *	OK	- Successful (returns always success code
 */

int clntExtractReplyCode (
	char	*buffer,    /* Pointer to an array containing the reply message (input) */
	int	*replyCode  /* reply code (output) */
	)
{
	/* extract the codefrom the server reply message */
   sscanf(buffer, "%d", replyCode);

   return (OK);
}  // end of clntExtractReplyCode()

void help()
{
  printf(" Command Name: user,  Syntax user username \n ");
  printf(" Command Name: pass,  Syntax pass password \n ");
  printf(" Command Name: quit,  Syntax quit \n ");
  printf(" Command Name: mkdir,  Syntax mkdir directory-name \n ");
  printf(" Command Name: rmdir,  rmdir directory-name \n ");
  printf(" Command Name: cd,  Syntax cd directory-name \n ");
  printf(" Command Name: dele,  Syntaxdele filename \n ");
  printf(" Command Name: pwd,  Syntax pwd \n ");
  printf(" Command Name: ls,  Syntax ls \n ");
  printf(" Command Name: stat,  Syntax stat \n ");
  printf(" Command Name: send,  Syntax send filename \n ");
  printf(" Command Name: recv,  Syntax recv filename \n ");
  printf(" Command Name: help,  Syntax user help \n ");
  fflush(stdin);
}

/* If it not a legal  command, return 1 */

int Notlegalcmd (char *cmd, char *argument, int count)
{

  if(count == 1) {

    int check =  !strcmp(cmd, "pwd")|| !strcmp(cmd, "ls")|| !strcmp(cmd, "stat") || !strcmp(cmd, "status") || !strcmp(cmd, "help") || !strcmp(cmd, "quit");
    return check;
  }
  else  if(count == 2) {

    int check= !strcmp(cmd, "user")|| !strcmp(cmd, "pass") || !strcmp(cmd, "quit") || !strcmp(cmd, "mkdir")|| !strcmp(cmd, "rmdir")|| !strcmp(cmd, "cd")|| !strcmp(cmd, "dele")||!strcmp(cmd, "send") || !strcmp(cmd, "recv")|| !strcmp(cmd, "put") || !strcmp(cmd, "get");
 
  return check;
 }
  else {

    printf("Error. Something went wrong here\n");
    return 1;
  }   

 
}
// returns number of words in str
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
        int reuse = 1;
        if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(reuse)) < 0)
          perror("setsockopt(SO_REUSEADDR) failed");
	/*initialize memory of svcAddr structure to zero. */
	memset((char *)&svcAddr,0, sizeof(svcAddr));

	/* initialize svcAddr to have server IP address and server listen port#. */
	svcAddr.sin_family = AF_INET;
	svcAddr.sin_addr.s_addr=htonl(INADDR_ANY);  /* server IP address */
	svcAddr.sin_port=htons(DATA_CONNECTION_PORT);    /* Clients listen Data port # */

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
//Data transfer function.
int dataTransfer(char * cmd, char * filename, char *type) {

  int listenSocket;   /* listening server ftp socket for client connect request */
  int ccSocket;        /* Control connection socket - to be used in all client communication */
  int status;


  /*initialize client listen socket*/
  printf("Initialize ftp data socket\n");	/* changed text */

  status=svcInitServer(&listenSocket);
  if(status != 0)
    {
      printf("Exiting client ftp due to svcInitServer returned error\n");
      exit(status);
    }


  printf("ftp  data socket is waiting to accept connection\n");

  /* wait until connection request comes from client ftp */
  ccSocket = accept(listenSocket, NULL, NULL);

  printf("Client came out of accept() function \n");

  if(ccSocket < 0)
    {
      perror("cannot accept connection:");
      printf("Client ftp is terminating after closing listen socket.\n");
      close(listenSocket);  /* close listen socket */
      return (ER_ACCEPT_FAILED);  // error exist
    }

  printf("Connected to the server \n");
  if(!strcmp(type ,"write")) {
    printf("Ready to get the file. \n");
    /*open the file to write */
    /*If a file exists already, We quit because we dont want to overwrite it. */
    /*Check the existence of the file */
    if( access( filename, F_OK ) != -1 ) {
      /*File exists*/
      printf("File %s already exist. Please rename or move it first \n",filename);
      close(ccSocket);   
      printf("Closing data listen socket.\n");
      close(listenSocket);  /*close data listen socket */
      return 0;
    } else {
      /*File does not exist. Open to write.*/
      printf("Writing file %s \n",filename);
      FILE* fd = fopen(filename, "wr");
              
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

        memset(recvBuff,'\0',sizeof(recvBuff));/* Cleanup everytime */
        int bytes_read = read(ccSocket, recvBuff, sizeof(recvBuff));
        if (bytes_read == 0) // We're done reading from the socket
          break;

        if (bytes_read < 0) {
          printf("Unknown error in reading the socket.\n");
          break;
        }
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
      printf("Closing data listen socket.\n");
      close(listenSocket);  /*close data listen socket */

      return 1;
    
    }
  }
  else if(!strcmp(type ,"read")){
    if( access( filename, R_OK ) == -1 ) {
      /*File doest not exist or does not have read permission. */
      printf("File %s doest not exist or does not have read permission. \n",filename);
      close(ccSocket);   
      printf("Closing data listen socket.\n");
      close(listenSocket);  /*close data listen socket */
      return 0;
    } else {
      /*File does exist. Open to read.*/
      printf("Reading file %s \n",filename);

      //mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
      int fd = open(filename, O_RDONLY);
              
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





      close(ccSocket);  
      printf("Closing data listen socket.\n");
      close(listenSocket);  /*close data listen socket */

      return 1; 
    }


  }
  else {
    printf("Error: Something went wrong here.\n");
      printf("Closing data listen socket.\n");
      close(listenSocket);  /*close data listen socket */
    exit(0);
  }

      printf("Closing data listen socket.\n");
      close(listenSocket);  /*close data listen socket */
  return 1;

}
