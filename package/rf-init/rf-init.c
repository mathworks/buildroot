/*
 * mw-rf-init application
 *
 * Initialize RFTOOL with parameters read from cfg file at boot-time
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define TXT_LINE_SIZE 4096
#define RPLY_LINE_SIZE 4096


typedef struct {
	int socket_desc_ctrl;
	int socket_desc_data;
	struct sockaddr_in rftool_ctrl;
	struct sockaddr_in rftool_data;
} socketStruct ;

char logbuf[RPLY_LINE_SIZE] = {0};

int setupComms(socketStruct *socketInput, FILE* fhlog);
void writeToLog(char * inputMsg,FILE* fh);


int main(int argc, char **argv)
//int main()
{
    int status = 0;
    socketStruct rftool_socket;
    char rftool_reply[RPLY_LINE_SIZE];
    FILE* fh = NULL;
    FILE* fhlog = NULL;
    char textbuf[TXT_LINE_SIZE] = {0};
    char blank_line[] = "\n";
    char pause_line[] = "PAUSE\n";
//    char * CONFIG_FILE_LOC = "/mnt/hdlcoder_rd/RF_Init.cfg";
    char CONFIG_FILE_LOC[64] = "/mnt/hdlcoder_rd/RF_Init.cfg";
    char RF_INIT_LOG_LOC[] = "/mnt/rf_init.log";

  

	if (argc == 1)
    {    
		printf("------usage rf-init < RFDC initialization file name >------------ : \n");
		printf(" No RFDC configuration file provide, default configuration file %s will be used\n", CONFIG_FILE_LOC);
       
    } 

	if (argc == 2) {
        sprintf(CONFIG_FILE_LOC, argv[1]);
		printf("Configuration files name provided : %s\n", CONFIG_FILE_LOC);
	}

    printf("Opening file %s\n", CONFIG_FILE_LOC);

    fh = fopen(CONFIG_FILE_LOC,"r");
    fhlog = fopen(RF_INIT_LOG_LOC,"w");

    if (fh == NULL)
    {
    	sprintf(logbuf,"rf_init: Could not locate %s ! Exiting...\n",CONFIG_FILE_LOC);
    	writeToLog(logbuf,fhlog);
        goto TERM_ERR;
    }

    if (setupComms(&rftool_socket,fhlog) < 0)
    {
    	sprintf(logbuf,"rf_init: Could not connect to RFTOOL. Exiting...\n");
    	perror(logbuf);
    	writeToLog(logbuf,fhlog);

        fclose(fh);
        goto TERM_ERR;
    }

    while (!feof(fh))
    {
        fgets(textbuf, sizeof(textbuf), fh);
        int compare2 = strncmp(textbuf,blank_line, 2);
        int compare3 = strcmp(textbuf,pause_line);
        if (compare2==0)
        {
            printf("-----SKIP BLANK LINE-----..\n");

        }
        else if (compare3 == 0)
        {
            printf("rf_init: PAUSING \n");
            sleep(1);
        }
        else
        {
            sprintf(logbuf,"rf_init: SENDING: %s",textbuf);
            printf("%s",logbuf);
            if ( send(rftool_socket.socket_desc_ctrl,
						textbuf,
						strlen(textbuf),
						MSG_CONFIRM) < 0 )
            {
            	perror("rf_init: Failed to send command to RFTOOL");
            }

            fwrite(textbuf,strlen(textbuf),1,fhlog);

            //look for ack
            status = recv(rftool_socket.socket_desc_ctrl,rftool_reply,RPLY_LINE_SIZE,0);
            if (status < 0)
            {
           	perror("rf_init: Failed to get ack packet from RFTOOL");
                fclose(fh);
    		goto TERM_ERR;

            }
            else
            {
            	sprintf(logbuf,"rf_init: RECEIVED: %.*s \n",status,rftool_reply);
        	writeToLog(logbuf,fhlog);
            }
        }

    }
    printf("rf_init: Flushing TCP/IP read buffer...\n");
    status = 1;

    while(status>0)
    {   //empty buffer until we get 0 bytes back (timed out..)
    	status = recv(rftool_socket.socket_desc_ctrl,rftool_reply,2048,0);
    	if (status > 0)
    	{
    		sprintf(logbuf,"rf_init: RECEIVED: %.*s \n",status,rftool_reply);
    		writeToLog(logbuf,fhlog);
    	}

     }

    writeToLog("rf_init: finished writing to rftool \n",fhlog);


TERM_ERR:
    fflush(fhlog);
    fclose(fhlog);
    return(-1);

fclose(fh);
fflush(fhlog);
fclose(fhlog);
return(0);
}

int setupComms(socketStruct *socketInput, FILE * fhlog)
{
	struct timeval tv;
	tv.tv_sec = 12;
	tv.tv_usec = 0;
	int MaxRetry = 20;
	int err = 0;

	//Create socket
	socketInput->socket_desc_ctrl = socket(AF_INET , SOCK_STREAM , 0);
	socketInput->socket_desc_data = socket(AF_INET , SOCK_STREAM , 0);
	if (socketInput->socket_desc_ctrl == -1)
	{
            sprintf(logbuf,"rf_init: Could not create socket");
	    writeToLog(logbuf,fhlog);
	}

	socketInput->rftool_ctrl.sin_addr.s_addr = inet_addr("127.0.0.1");
	socketInput->rftool_ctrl.sin_family = AF_INET;
	socketInput->rftool_ctrl.sin_port = htons( 8081 );

	socketInput->rftool_data.sin_addr.s_addr = inet_addr("127.0.0.1");
	socketInput->rftool_data.sin_family = AF_INET;
	socketInput->rftool_data.sin_port = htons( 8082 );

	//Connect to control plane
	err = -1;
	int connCount = 0;
	while(err<0 && connCount<MaxRetry)
	{
	    err = connect(socketInput->socket_desc_ctrl ,
	    (struct sockaddr *)&socketInput->rftool_ctrl ,
	    sizeof(socketInput->rftool_ctrl));
	    sleep(1);
	    connCount++;

	    if (err<0)
	    {
	        sprintf(logbuf,"rf_init: Could not connect to RFTOOL...Iteration:%d \n",connCount);
    		writeToLog(logbuf,fhlog);
	    }

	}

	if (err<0 && connCount>=MaxRetry)
	{
            sprintf(logbuf,"rf_init: Failed to establish connection after %d retries \n",connCount);
	    writeToLog(logbuf,fhlog);
	    return(-1);
	}

	if (connect(socketInput->socket_desc_data , (struct sockaddr *)&socketInput->rftool_data , sizeof(socketInput->rftool_data)) < 0)
	{
            sprintf(logbuf,"rf_init: Data plane connection error");
	    writeToLog(logbuf,fhlog);
	    return(-1);
	}
	else
	{
            sprintf(logbuf,"rf_init: Connected to data plane \n");
	    writeToLog(logbuf,fhlog);
	}

	// Apply timeout
	setsockopt(socketInput->socket_desc_ctrl, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
	setsockopt(socketInput->socket_desc_data, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);

	return(err);

}


void writeToLog(char * inputMsg,FILE* fh)
{
	printf("%s",inputMsg);
	fwrite(inputMsg,strlen(inputMsg),1,fh);
}
