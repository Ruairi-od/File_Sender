/*
 * Server.h
 *
 *  Created on: 16 Dec 2015
 *      Author: ruairi
 */

#ifndef SERVER_H_
#define SERVER_H_

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <sys/stat.h>

#define SERV_PORT 32980  // port to be used by server
#define DOWNLOAD 1 //Definition for setting download mode
#define UPLOAD 2 //Definition for setting upload mode

long timeSet(float limit);
int timeUp(long timeLimit);
void error(const char *msg);
long rec_file(int cSocket, char *fName, char *client_ip);
long send_file(int cSocket, char *fName);

#endif /* SERVER_H_ */
