/*
 * Server.c
 *
 *  Created on: 16 Dec 2015
 *      Author: Ruairi O'Donoghue
 */
#include "Server.h"

/*
 *   Function to set time limit at a point in the future.
 *   limit is time limit in seconds (from now)
 */
long timeSet(float limit)
{
	long timeLimit = clock() + (long)(limit * CLOCKS_PER_SEC);
	return timeLimit;
}

/*
 *   Function to check if time limit has elapsed.
 *   timer  is timer variable to check
 *   returns 1 if time has reached or exceeded limit,
 *   0 if time has not yet reached limit.
 */
int timeUp(long timeLimit)
{
	if (clock() < timeLimit) return 0;  // still within limit
	else return 1;  // time limit has been reached or exceeded
}

void error(const char *msg)
{
	perror(msg);
	exit(1);
}

int kbhit(void)
{
	struct timeval tv;
	fd_set read_fd;

	/* Do not wait at all, not even a microsecond */
	tv.tv_sec=0;
	tv.tv_usec=0;

	/* Must be done first to initialize read_fd */
	FD_ZERO(&read_fd);

	/* Makes select() ask if input is ready:
	 * 0 is the file descriptor for stdin    */
	FD_SET(0,&read_fd);

	/* The first parameter is the number of the
	 * largest file descriptor to check + 1. */
	if(select(1, &read_fd,NULL, /*No writes*/NULL, /*No exceptions*/&tv) == -1)
		return 0;  /* An error occured */

	/*  read_fd now holds a bit map of files that are
	 * readable. We test the entry for the standard
	 * input (file 0). */

	if(FD_ISSET(0,&read_fd))
		/* Character pending on stdin */
		return 1;

	/* no characters were pending */
	return 0;
}

/*
 * The rec_file function is used when the client has requested to upload a file to the server.
 * It returns the amount of data it has wrote to the file upon completion.
 * If this number is negative an error has occured somewhere in the function and an error message should print to the output.
 * The server user is prompted to set a name for the folder which will store the clients uploads.
 * If no input is detected after this prompt within 5 seconds the function continues setting the folder name
 * to the clients ip.
 * The function then attempts to recieve the header and the first part of the file from the client.
 * If the server receives the file with no error it will attempt to write to the file.
 * The function will then loop and attempt to recieve more data and write it to the file.
 * A loading percentage is displayed for the server user to see how much of the data has been recieved and written to the file.
 * The total number of bytes received is printed to the screen and the function returns the number of bytes written to the file
 * to the main function.
 */

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
			printf("Problem receiving\n");
			error("");
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
					printf("ERROR: Problem with file size from client.\n");
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

/*
 * The send_file function is used when the client has requested to download a file from the server.
 * The function first checks if the server has the file that has been requested by the client.
 * If it does not an error message is printed and -1 is sent to the client to signify it does not have the file.
 * If the file does exist the server checks the length of the file and prints the file size.
 * The server then attempts to read the file and extract the data from it.
 * The file is then closed and the header is created for the sending of the data.
 * The server then attempts to send the header to the client.
 * Then if the header was successful the data of the file is sent.
 */

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


/*
 * The getRequest function is used to handle requests from the client.
 * It recieves the first transmission from the client which contains whether or not the client wants to upload or download.
 * It also gets the filename from the client in this transmission.
 * It returns which mode the server is operating so that the correct function can be called in main.
 */

int getRequest(int cSocket, char *fName)
{
	//The function takes two variables, the socket being used and the pointer to the variable where the filename will be stored.
	int mode, nRx, j=1, k=0;
	char request[100];

	nRx = recv(cSocket, request, 100, 0);//Attempts to receive info from client. nRx is number of bytes received.
	if( nRx == -1)//If error receiving data.
	{
		printf("Problem receiving\n");//Print error message.
		error("");
		return -1;//Exit function.
	}
	else if( nRx < 0)//If error receiving from client
	{
		printf("Problem receiving, connection closed by client\n");//Print error message.
		error("");
		return -1;//Exit function.
	}
	else if (nRx == 0)  // connection closing
	{
		printf("Connection closed by server\n");
		return -1;//Exit function.
	}
	else//We have recevied some data from the client.
	{
		switch(request[0])//Switch loop to check which mode the client has selected.
		{
		case 'd': //If "d" was first character in transmission the client wants to download from server.
			mode = DOWNLOAD;//Set mode to download.
			printf("Download requested\n");
			break;//Exit switch.
		case 'u': //If "u" was first character in transmission the client wants to upload to the server.
			mode = UPLOAD;//Set mode to upload.
			printf("Upload requested\n");
			break;//Exit Switch.
		default ://If unknown character is received.
			printf("ERROR in request\n");//Print error message.
			return -1;//Exit function.
		}
		while(request[j] != 3)//Look for ETX marker in transmission.
		{
			fName[k]=request[j];//Extract filename from transmission.
			j++;
			k++;
		}
		fName[k] = 0;//Set null marker at the end of the filename.
		printf("Filename is %s\n", fName);//Print filename for user.
	}
	return mode;//Return the mode that was selected.
}

int main(int argc, char *argv[])
{
	int serverSocket, cSocket;
	socklen_t clilen;
	struct sockaddr_in client, serv;
	memset(&serv, 0, sizeof(serv));
	serverSocket = socket(AF_INET, SOCK_STREAM, 0);
	int mode=0;
	char fName[100];
	long bytes_rec=0, bytes_sent=0;
	char client_ip[50];

	if(serverSocket < 0)
	{
		error("ERROR opening socket\n");
	} else
	{
		printf("Socket Opened successfully\n");
	}
	bzero((char *) &serv, sizeof(serv));
	serv.sin_family = AF_INET;
	serv.sin_addr.s_addr = INADDR_ANY;
	serv.sin_port = htons(SERV_PORT);
	if (bind(serverSocket, (struct sockaddr *) &serv, sizeof(serv)) < 0)
	{
		error("ERROR on binding");
	} else
	{
		printf("Socket Bound\n");
	}
	listen(serverSocket, 5);
	printf("Listening on port %d\n", SERV_PORT);
	clilen = sizeof(client);
	printf("Accept\n");
	cSocket = accept(serverSocket, (struct sockaddr *) &client, &clilen);
	//sprintf( client_ip, "%d", client.sin_addr );
	if (cSocket < 0) {
		error("ERROR on accept");
	} else
	{
		printf("Accepted connection on port %d\n", client.sin_port);
	}
	mode = getRequest(cSocket, fName);
	if(mode == DOWNLOAD)//If the client wants to download from the server.
	{
		printf("Download Start\n");
		bytes_sent = send_file(cSocket, fName);//Call the send_file function.
		printf("File Sent\n");
		printf("Total bytes sent = %ld\n", bytes_sent);//Print number of bytes sent to client.
	}
	else if (mode == UPLOAD) //If the client wants to upload to server.
	{
		printf("Upload Start\n");
		bytes_rec = rec_file(cSocket, fName, "localhost");//Call the rec_file function.
		printf("Upload Completed\n");
		printf("Bytes written = %ld\n", bytes_rec);//Print number of bytes written to file.
	}
	printf("Connection closing...\n");
	close(cSocket);
	close(serverSocket);
	return 0;
}
