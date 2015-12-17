#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>

#define DOWNLOAD 1
#define UPLOAD 2

void error(const char *msg)
{
	perror(msg);
	printf("Error closing\n");
	exit(0);
}



int main(int argc, char *argv[])
{
	int sockfd, portno, n;
	struct sockaddr_in serv_addr;
	struct hostent *server;
	int retVal, nRx, nIn, j;
	int endLine = 0, stop = 0;  // flags to control loops
	char serverIP[20];      // IP address of server
	int serverPort;         // port used by server
	char request[100];      // array to hold user input
	char down_up[100];     //array to differentiate between uploading or downloading
	char send_request[100];  //array that is sent
	char infoStr[50];       //string containing data of file being uploaded
	char header[100];       //array to hold received header
	char reply[10000];      // array to hold received bytes
	char *data;             // pointer to data being sent
	int k=0;
	int flag=0;
	int mode;               //uploading mode or downloading mode
	int nBytes;
	FILE *fp;
	int i;

	char buffer[256];
//	if (argc < 3) {
//		fprintf(stderr,"usage %s hostname port\n", argv[0]);
//		exit(0);
//	}
//	portno = atoi(argv[2]);
	portno = 32980;
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0)
		error("ERROR opening socket");
//	server = gethostbyname(argv[1]);
	server = gethostbyname("localhost");
	if (server == NULL) {
		fprintf(stderr,"ERROR, no such host\n");
		exit(0);
	}
	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	bcopy((char *)server->h_addr,
			(char *)&serv_addr.sin_addr.s_addr,
			server->h_length);
	serv_addr.sin_port = htons(portno);
	if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0)
		error("ERROR connecting");
	while(stop == 0)

	{
		// Main loop to send messages and receive responses
		// This example assumes client will send first
		do{
			printf("Do you wish to download[d] or upload[u]? ");
			fgets(down_up, sizeof(down_up), stdin); //saves data into character array


			if(down_up[0]==('D')||down_up[0]==('d'))
			{
				send_request[0] = 'd';
				mode = DOWNLOAD;
			}

			else if(down_up[0]==('U')||down_up[0]==('u'))
			{
				send_request[0] = 'u';
				mode = UPLOAD;
			}
			else  //if other character is entered, error occurs, try again
			{
				printf("unknown characters entered\n Try again\n\n");

			}
		}
		while((mode!=UPLOAD)&&(mode!=DOWNLOAD));

		send_request[1]= '\0'; //put null character to ensure array only has d or u

		printf("Enter filename (max 90 bytes, $ to end program): ");
		fgets(request, sizeof(request), stdin);  // read in the string
		nIn = strlen(request);  //find the length
		request[nIn+1]=0; //put a null character to end the array
		strcat(send_request,request);  //join arrays together
		nIn = strlen(send_request);  //find the length
		printf("print request %s\n", send_request);
		send_request[nIn-1] = 3;  // replace null character with 3, our end of text marker
		send_request[nIn] = 0;



		if (request[0] == '$') stop = 1;  // set stop flag if $ entered
		if (stop == 1) printf("Closing connection as requested...\n");
		else  // send the message and try to receive a reply
		{
			// send() arguments: socket handle, array of bytes to send,
			// number of bytes to send, and last argument of 0.

			retVal= send(sockfd, send_request, nIn+2, 0);  // send nIn+1 bytes
			// retVal will be number of bytes sent, or error indicator



			if( retVal == -1) // check for error
			{
				printf("*** Error sending\n");
				error("");
			}
			else printf("Sent %d bytes, waiting for reply...\n", retVal);
			endLine = 0;

			if(mode == DOWNLOAD)  //if we are downloading
			{
				do     // loop to receive entire reply, terminated by 3/lf
				{
					// Try to receive some bytes
					// recv() arguments: socket handle, array to hold rx bytes,
					// maximum number of bytes to receive, last argument 0.
					nRx = recv(sockfd, reply, 100, 0);
					// nRx will be number of bytes received, or error indicator

					if( nRx == -1)  // check for error
					{
						printf("Problem receiving\n");
						error("");
						stop = 1;  // exit the loop if problem
					}
					else if (nRx == 0)  // connection closed
					{
						printf("Connection closed by server\n");
						stop = 1;
					}
					else if (nRx > 0)  // we got some data
					{
						//printf("recieved %d\n", nRx);
						/*for (k = 0; k < nRx; k++)
                            {
                                //printf("%c", reply[k]);  // print each character

                            }
						 */
					}
					if(flag==0) //do this once, as the size of the file
						//should be at the start of the data we receive
					{
						j=0;
						//printf("find size of file\n");
						while(reply[j] != 3  && j<nRx)
						{
							header[j]=reply[j]; //finds size of file
							//3 is end marker for size of file
							j++;
							//size of file in string received

						}
						j++;
						header[j]=0;
						flag=1;
						//printf("stopped at %d\n", j);
						int file_size= atoi(header);
						//printf("file size = %d\n", file_size);

						if(file_size<=0)
						{
							printf("SERVER: FILE DOES NOT EXIST\n");
							stop=1; //cannot continue, server doesnt have file
						}
						else  //recieve first part of file that includes the size of file
						{


							//printf("Server has file\n file size = %d\n", file_size);
							//printf("printing file:\n");
							fp=fopen(request,"wb"); //create/open a file with the same name as the one created

							if(nRx-j>1) // write in file
							{
								fwrite(reply+j, 1, nRx-j, fp);
							}
							printf("NRX = %d, j= %d\n", nRx,j);



						}

					} //end of stuff to do once only
					else // stuff to do repeatedly
					{
						fwrite(reply, 1, nRx, fp); //write in file
					}
				}
				while  (stop == 0);
			} else if(mode == UPLOAD) // UPLOAD mode
			{
				// Try to open the file
				printf("\nOpening %s for binary read\n", request);
				request[strlen(request)-1]=0;
				for(i=0;i<strlen(request);i++)
				{
					printf("i:%d  #c:%d  c:%c\n", i, request[i], request[i]);
				}
				fp = fopen(request, "rb");  // open for binary read

				if (fp == NULL) //report error if there is a problem opening file
				{
					perror("Error opening file");
					printf("errno = %d\n", errno);
					send(sockfd, "-1", 2, 0);
					return 1;
				}
				else
					printf("file found");

				// Find size of file
				retVal = fseek(fp, 0, SEEK_END);  // set current position to end of file
				if (retVal != 0)  // there was an error finding the size of the file
				{
					perror("Error in fseek");
					printf("errno = %d\n", errno);
					fclose (fp);
					return 2;
				}
				nBytes = ftell(fp);         // find out what current position is
				printf("\nFile size is %d bytes\n", nBytes);  // print it

				//**********************************************************************

				// To read the file, start at the beginning
				retVal = fseek(fp, 0, SEEK_SET);   // set current position to start of file
				if (retVal != 0)  // there was an error starting at the beginning
				{
					perror("Error in fseek");
					printf("errno = %d\n", errno);
					fclose (fp);
					return 3;
				}
				printf("Attempting to read file\n");
				// Try to read some data and print it (just for demonstration)
				data = malloc(1 + (sizeof(char) * nBytes ));
				retVal = (int) fread(data, 1, nBytes, fp);  // read the full file.
				if (ferror(fp))  // check for error
				{
					perror("Error reading input file");
					printf("errno = %d\n", errno);
					fclose(fp);
					return 4;
				}

				// When finished, close the file
				fclose (fp);
				printf("\nFile closed\n");

				// Convert byte count to a string, in decimal
				sprintf(infoStr,"%d", nBytes);

				int lenStr = strlen(infoStr);  // get length of this string
				infoStr[lenStr] = 3;
				lenStr++;
				infoStr[lenStr] = 0;

				printf("\nFile size as string: %s, length %d\n", infoStr, lenStr);


				retVal = send(sockfd, infoStr, lenStr, 0); //send the data to client
				printf("First Send = %s length = %d\n", infoStr, lenStr);
				printf("Stop = %d\n", stop);
				printf("Endline = %d\n", endLine);




				//************************************************************************************
				if (endLine == 0)  // LF character has been received
				{

					if (stop == 0)
					{
						// send() arguments: socket handle, array of bytes to send,
						// number of bytes to send, and last argument of 0.
						printf("Attempting to send data\n");
						retVal = send(sockfd, data, nBytes, 0);  // send nIn+2 bytes
						// retVal will be number of bytes sent, or error indicator

						if( retVal == -1)  // check for error
						{
							printf("*** Error sending response\n");
							error("");
						}
						else
						{
							printf("Sent %d bytes, waiting for reply...\n", retVal);
							stop=1;
						}

					}  // end if stop == 0

				}  // end if endline == 1
			} else
			{
				printf("Error neither upload or download selected\n");
				exit(EXIT_FAILURE);
			}
			// continue until endline or error or connection closed
			// if it was endline, the outer loop should continue

		} // end else (not stop)
	}
	printf("Closing Connection\n");
	close(sockfd);
	return 0;
}
