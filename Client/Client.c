/*
 * Client_Rev.c
 *
 *  Created on: 17 Dec 2015
 *      Author: Ruairi O'Donoghue
 */

#include "Client.h"

void error(const char *msg)
{
	perror(msg);
	printf("Error closing\n");
	exit(0);
}

int getMode(int cSocket, char *fName)
{
	char request[51], temp[50];
	char choice;
	int  mode=0, tRx, i;

	do
	{
		printf("Do you wish to download[d] or upload[u]? ");
		choice = fgetc(stdin);
		if (choice == 'u' || choice == 'U')
		{
			request[0] = 'u';
			mode = UPLOAD;
			printf("Upload selected\n");
		} else if (choice == 'd' || choice == 'D')
		{
			request[0] = 'd';
			mode = DOWNLOAD;
			printf("Download selected\n");
		} else
		{
			printf("Unknown character entered. Please try again.\n");
		}
	} while(mode==0);

	printf("Enter the filename:");
	__fpurge(stdin);
	fgets(temp, sizeof(temp), stdin);
	strtok(temp, "\n");
	memcpy(fName, temp, strlen(temp));
	strcat(request, temp);
	request[strlen(request)] = 3;
	//	for(i=0;i<strlen(request);i++)
	//	{
	//		printf("I:%d  #C:%d  C:%c\n", i, request[i], request[i]);
	//	}
	tRx = send(cSocket, request, strlen(request), 0);
	if(tRx == -1)
	{
		error("Error: Sending Filename\n");
	} else
	{
		printf("Sent %d bytes for filename and mode\n", tRx);
	}
	return mode;
}

long rec_file(int cSocket, char *fName, char *client_ip)
{
	//The function takes three arguments, the socket being used, the filename, and the ip of the client.
	int nRx=0, flag=0, stop=0, j=0, file_size=0, last_percent=0;
	float percent = 0.0;
	long bytes_written=0;
	long bytes_rec=0;
	char reply[1400];
	char f_size[100];
	FILE *fp;
	char nickname[10];
	double perc_bytes=0, perc_size=0;
	int perc_rounded=0;
	long timer;
	int i;


	printf("%s is the default folder name for this ip.\n", client_ip);
//	printf("Would you like to set a folder name for %s? [y]es or [n]o ", client_ip);
//	timer = timeSet(5);
//The user is prompted to set a folder name for the client and the timer is set to 5 seconds.
//	while(timeUp(timer) != 1)//While the time limit is not reached
//	{
//		if(kbhit())//If any keyboard input is detected.
//		{
//			fgets(nickname, sizeof(nickname), stdin);//Variable to hold answer for prompt.
//			if(nickname[0]==('Y')||nickname[0]==('y'))//If the user says yes
//			{
//				printf("Please set a name for %s: ", client_ip);
//				fgets(client_ip, sizeof(client_ip), stdin	);//Set the folder name for the client.
//			}
//			break;
//		}
//	}
//	printf("\n");
	do
	{
		nRx = recv(cSocket, reply, 1400, 0);//Attempt to recieve data from client in blocks of 1400.
		if( nRx == -1)//Error receiving.
		{
			error("Problem receiving\n");
			bytes_written = -1;
			stop = -1;//The error is printed and a flag is set to -1 to exit function and show error.
		}
		else if (nRx == 0)//Connection was closed by server.
		{
			printf("Connection closed by server\n");
			stop = 1;//Flag set to exit function.
		}
		else if (nRx > 0)//We have received some data
		{
			bytes_rec += nRx;//Update number of bytes received
			if(flag==0)//If this is the first block of data containing header
			{
				while(reply[j] != 3  && j<nRx)//Looking for end marker to signal end of header. Marker is ETX symbol.
				{
					f_size[j]=reply[j];//Gets the file size from the header in string form
					j++;

				}
				f_size[j]=0;//Sets null marker in place of ETX marker.
				flag=1;//Sets flag so this statement is not repeated
				file_size= atoi(f_size);//Gets the file size from the string and puts it as an integer
				printf("File size = %d\n", file_size);//Prints file size

				if(file_size<=0)//If the file size is a negative number error by client sending file
				{
					error("ERROR: Problem with file size from client.\n");
					bytes_written = -2;
					stop=1;//Flag set to exit function.
				}
				else
				{
					if (mkdir(client_ip, S_IRWXU | S_IRWXG | S_IRWXO	) == -1)//Try to create folder for client.
					{
						printf("Warning! Directory may already exist. CreateDirectory failed.\n");
						//Error creating folder or folder exists already.
					}
					if(chdir("localhost") == -1)//Try to change the working directory to desired folder.
					{
						printf("Error changing directory. Directory may not exist.\n");
						//Error changing directory. May occur if bad directory name chosen by user.
					}
					fp=fopen(fName,"wb");//Create the file with given filename from client.
					j++;
					if(nRx-j>1)
					{
						printf("%d bytes recieved in the header\n", j);
						fwrite(reply+j, 1, nRx-j, fp);//Write first part of data but not header.
						bytes_written += nRx-j;//Update number of bytes written to file.
					}
				}
			}
			else//This is not the first transimission of data.
			{
				fwrite(reply, 1, nRx, fp);//Write the block of data to file.
				bytes_written += nRx;//Update number of bytes written to file.
				if(bytes_written == file_size)//Check if file is complete.
					stop=1;//If yes flag to end function.
				perc_bytes = (double) bytes_written;//Set variables used to display percentage
				perc_size= (double) file_size;
				percent = (100.0*(perc_bytes/perc_size));//Get percent by number of bytes written divided by file size by 100.
				perc_rounded = (int)(percent);//Round upwards to nearest percent.
				if(perc_rounded % 10 == 0 && perc_rounded != last_percent)//Only display percentages every 10 and dont display twice.
				{
					printf("%d%% Complete\n", perc_rounded);//Print the percentage to screen.
					last_percent = perc_rounded;//Update most recently displayed percentage.
				}
			}
		}
	}
	while  (stop == 0);//Check flag which ends loop
	if(stop >= 0)//If we did open the file
		fclose(fp);//Close the file

	printf("Bytes recieved = %ld\n", bytes_rec);//Print number of bytes received.
	return bytes_written;//Return number of bytes written to file to main function.
}

long send_file(int cSocket, char *fName)
{
	//The function takes two arguments the socket being used and the file name requested.
	int retVal, stop=0, lenStr;
	long nBytes, bytes_sent=0;
	char *data;
	char infoStr[20];
	FILE *fp;


	printf("Attempting to open %s for binary read\n", fName);
	fp = fopen(fName, "rb");//The file requested is checked to see if it exists and is accessible.
	if (fp == NULL)//If the file does not exist or is inaccessible.
	{
		perror("Error opening file");//Print error.
		printf("errno = %d\n", errno);
		send(cSocket, "-1", 2, 0);//Send -1 to client to show problem accessing file.
		return -1;//Exit function.
	}
	else
		printf("%s found", fName);//File is accessible

	retVal = fseek(fp, 0, SEEK_END);  //Set current position to end of file
	if (retVal != 0)  //There was an error
	{
		perror("Error in fseek");//Display error message
		printf("errno = %d\n", errno);
		fclose (fp);//Close the file.
		return -2;//Exit the function.
	}
	nBytes = ftell(fp);//Find out what current position is
	printf("\nFile size is %ld bytes\n", nBytes);//Print the sie of the file in bytes.
	retVal = fseek(fp, 0, SEEK_SET);//Set current position to start of file
	if (retVal != 0)//There was an error
	{
		perror("Error in fseek");//Print error message.
		printf("errno = %d\n", errno);
		fclose (fp);//Close file.
		return -3;//Exit function.
	}
	printf("Attempting to read file\n");
	data = malloc(1 + (sizeof(char) * nBytes ));//Create array the size of the file.
	retVal = (int) fread(data, 1, nBytes, fp);//Read the full file.
	if (ferror(fp))//If there was an error reading the file.
	{
		perror("Error reading input file");//Print error message.
		printf("errno = %d\n", errno);
		fclose(fp);//Close the file.
		return -4;//Exit the function.
	}
	if (retVal == nBytes)//The file was read successfully.
	{
		printf("File read successfully\n");
	}
	else
	{
		printf("\nError reading the file.\n");//Error reading file.
		fclose (fp);//Close file.
		return -5;//Exit function.
	}
	fclose (fp);//Close the file
	printf("File closed\n");
	sprintf(infoStr,"%lu", nBytes);//Convert file size from long integer to string so it can be sent to client.
	lenStr = strlen(infoStr);//Get the length of this string
	infoStr[lenStr] = 3;//Set ETX marker.
	lenStr++;//Update string length.
	infoStr[lenStr] = 0;//Set NULL marker.
	retVal = send(cSocket, infoStr, lenStr, 0);//Send header to client.
	if( retVal == -1)//Check for error sending header.
	{
		printf("*** Error sending header\n");//Print error.
		stop=1;//Set flag to not send rest of file.
		error("");//Print the error.
	}
	else
	{
		printf("Sent %d bytes in header\n", retVal);//Header sent successfully.
		bytes_sent += retVal;//Update number of bytes sent.
	}
	if (stop == 0)//If no error sending header.
	{
		retVal = send(cSocket, data, nBytes, 0);//Send the data of the file.
		if( retVal == -1)//Check for error sending data.
		{
			printf("*** Error sending response\n");//Print error message.
			error("");
		}
		else
		{
			printf("Sent %ld bytes as data\n", nBytes);//Print number of bytes sent as data.
			bytes_sent += nBytes;//Update number of bytes sent.
		}
	}
	free(data);//Free array used to hold data.
	return bytes_sent;//Return total number of bytes sent to client.
}

int main(int argc, char *argv[])
{
	int cSocket, portno=32980, i;
	struct sockaddr_in serv_addr;
	struct hostent *server;
	char fName[50];

	cSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (cSocket < 0)
	{
		error("ERROR opening socket");
	} else
	{
		printf("Socket opened successfully\n");
	}
	server = gethostbyname("localhost");
	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
	serv_addr.sin_port = htons(portno);
	if (connect(cSocket,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0)
	{
		error("ERROR connecting");
	} else
	{
		printf("Connected successfully\n");
	}
	if(getMode(cSocket, fName) == DOWNLOAD)
	{
		rec_file(cSocket, fName, "localhost");
	} else
	{
		send_file(cSocket, fName);
	}
	printf("Closing Connection\n");
	close(cSocket);


	return 0;
}

