#include <stdio.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string.h>
//claires ip is 192.168.2.19, port no = 32980
//kevs ip 78.19.64.1

#define DOWNLOAD 1
#define UPLOAD 2

void printError(void);  // function to display error messages

int main()
{
    WSADATA wsaData;  // create structure to hold winsock data
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
    FILE *fp;               //pointer to file


    // Initialise winsock, version 2.2, giving pointer to data structure
    retVal = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (retVal != 0)  // check for error
    {
        printf("*** WSAStartup failed: %d\n", retVal);
        printError();
        return 1;
    }
    else printf("WSAStartup succeeded\n" );

    // Create a handle for a socket, to be used by the client
    SOCKET clientSocket = INVALID_SOCKET;  // handle called clientSocket

    // Create the socket, and assign it to the handle
    // AF_INET means IP version 4,
    // SOCK_STREAM means socket works with streams of bytes,
    // IPPROTO_TCP means TCP transport protocol.
    clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (clientSocket == INVALID_SOCKET)  // check for error
    {
        printf("*** Failed to create socket\n");
        printError();
        stop = 1;
    }
    else printf("Socket created\n" );

    // Get the details of the server from the user
    printf("Enter IP address of server: ");
    scanf("%20s", serverIP);  // get IP address as string

    printf("Enter port number: ");
    scanf("%d", &serverPort);     // get port number as integer

    gets(request);  // flush the endline from the input buffer

    // Build a structure to identify the service required
    // This has to contain the IP address and port of the server
    struct sockaddr_in service;  // IP address and port structure, named it service
    service.sin_family = AF_INET;  // specify IP version 4 family
    service.sin_addr.s_addr = inet_addr(serverIP);  // set IP address
    // function inet_addr() converts IP address string to 32-bit number
    service.sin_port = htons(serverPort);  // set port number
    // function htons() converts 16-bit integer to network format

    // Try to connect to the service required
    printf("Trying to connect to %s on port %d\n", serverIP, serverPort);
    retVal = connect(clientSocket, (SOCKADDR *) &service, sizeof(service));
    if( retVal != 0)  // check for error
    {
        printf("*** Error connecting\n");
        printError();
        stop = 1;  // make sure we do not go into the while loop
    }
    else printf("Connected!\n");

    while(stop == 0)

    {
        // Main loop to send messages and receive responses
        // This example assumes client will send first
        do{
            printf("Do you wish to download[d] or upload[u]? ");
            gets(down_up); //saves data into character array


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
        gets(request);  // read in the string
        nIn = strlen(request);  //find the length
        request[nIn+1]=0; //put a null character to end the array
        strcat(send_request,request);  //join arrays together
        nIn = strlen(send_request);  //find the length
        printf("print request %s\n", send_request);
        send_request[nIn] = 3;  // replace null character with 3, our end of text marker
        send_request[nIn+1] = 0;



        if (request[0] == '$') stop = 1;  // set stop flag if $ entered
        if (stop == 1) printf("Closing connection as requested...\n");
        else  // send the message and try to receive a reply
        {
            // send() arguments: socket handle, array of bytes to send,
            // number of bytes to send, and last argument of 0.

            retVal= send(clientSocket, send_request, nIn+2, 0);  // send nIn+1 bytes
            // retVal will be number of bytes sent, or error indicator



            if( retVal == SOCKET_ERROR) // check for error
            {
                printf("*** Error sending\n");
                printError();
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
                    nRx = recv(clientSocket, reply, 100, 0);
                    // nRx will be number of bytes received, or error indicator

                    if( nRx == SOCKET_ERROR)  // check for error
                    {
                        printf("Problem receiving\n");
                        printError();
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
            fp = fopen(request, "rb");  // open for binary read
            if (fp == NULL) //report error if there is a problem opening file
            {
                perror("Error opening file");
                printf("errno = %d\n", errno);
                send(clientSocket, "-1", 2, 0);
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
            printf("\nFile size is %ld bytes\n", nBytes);  // print it

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
            if (retVal > 0 )  // we got some bytes, print them - this can be included for debugging purposes
            {
                // printf("\nRead first %d bytes: \n", retVal);
                /* for(i=0; i<retVal; i++)
                 {
                     bVal = (unsigned char) data[i]; // get value
                     printf("%d ", bVal);  // print one byte at a time
                     if (i%10 == 9) printf("\n");  // new line after 10
                     if (bVal < minB) minB = bVal;  // update minB
                     if (bVal > maxB) maxB = bVal;  // update maxB
                 }
                 */
                /*for(i=0; i<retVal; i++)
                {
                    bVal = (unsigned char) data[i]; // get value
                    printf("%d ", bVal);  // print one byte at a time
                    if (i%10 == 9) printf("\n");  // new line after 10
                    if (bVal < minB) minB = bVal;  // update minB
                    if (bVal > maxB) maxB = bVal;  // update maxB
                }
                */
                // print as string if it seems to be text
                /*if ((minB >= 10) && (maxB <= 127))
                {
                    data[retVal] = 0;  // add null marker to make string
                    printf("\nAs string:\n%s\n", data);
                }
                else printf("\n");  // just tidy up
                    */
            }
            else printf("\nNo data in file\n"); //the file was empty

            // When finished, close the file
            fclose (fp);
            printf("\nFile closed\n");

            // Convert byte count to a string, in decimal
            itoa(nBytes, infoStr, 10);  // non-standard function, but common

            int lenStr = strlen(infoStr);  // get length of this string
            infoStr[lenStr] = 3;
            lenStr++;
            infoStr[lenStr] = 0;

            printf("\nFile size as string: %s, length %d\n", infoStr, lenStr);


            retVal = send(clientSocket, infoStr, lenStr, 0); //send the data to client
            printf("First Send = %s length = %d\n", infoStr, lenStr);
            printf("Stop = %d\n", stop);
            printf("Endline = %d\n", endLine);




            //************************************************************************************

            // If the incoming message is finished (LF found),
            // then get our response from the user
            if (endLine == 0)  // LF character has been received
            {

                /*printf("Enter response (max 90 char, $ to close connection): ");
                gets(response);  // read in string
                // gets() reads until enter (CR), but does not put CR in string
                nIn = strlen(response);  //find the length
                response[nIn] = 13;  // replace null terminator with CR
                response[nIn+1] = 10;  // add LF
                if (response[0] == '$') stop = 1;  // set stop flag if $ entered
                */

                if (stop == 0)
                {
                    // send() arguments: socket handle, array of bytes to send,
                    // number of bytes to send, and last argument of 0.
                    printf("Attempting to send data\n");
                    retVal = send(clientSocket, data, nBytes, 0);  // send nIn+2 bytes
                    // retVal will be number of bytes sent, or error indicator

                    if( retVal == SOCKET_ERROR)  // check for error
                    {
                        printf("*** Error sending response\n");
                        printError();
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
        fclose(fp); // close file

    // Shut down the sending side of the TCP connection first
    retVal = shutdown(clientSocket, SD_SEND);
    if( retVal != 0)  // check for error
    {
        printf("*** Error shutting down sending\n");
        printError();
    }

    // Then close the socket
    retVal = closesocket(clientSocket);
    if( retVal != 0)  // check for error
    {
        printf("*** Error closing socket\n");
        printError();
    }
    else printf("Socket closed\n");

    // Finally clean up the winsock system
    retVal = WSACleanup();
    printf("WSACleanup returned %d\n",retVal);

    // Prompt for user input, so window stays open when run outside CodeBlocks
    printf("\nPress return to exit:");
    gets(request);
    return 0;
}

/* Function to print informative error messages
   when something goes wrong...  */
void printError(void)
{
    char lastError[1024];
    int errCode;

    errCode = WSAGetLastError();  // get the error code for the last error
    FormatMessage(
        FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        errCode,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        lastError,
        1024,
        NULL);  // convert error code to error message
    printf("WSA Error Code %d = %s\n", errCode, lastError);
}

