/*
 * Client_Rev.h
 *
 *  Created on: 17 Dec 2015
 *      Author: ruairi
 */

#ifndef CLIENT_REV_H_
#define CLIENT_REV_H_

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <sys/stat.h>

#define DOWNLOAD 1
#define UPLOAD 2

void error(const char *msg);
void chooseMode(int cSocket, char *request);
long send_file(int cSocket, char *fName);

#endif /* CLIENT_REV_H_ */
