#ifndef __SOCKCOMM_H
#define __SOCKCOMM_H

#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

const int LISTEN_BACKLOG = 5;

void ExitError(const char*);
int ConnectToServer(char*, int);
int SocketInit(int);
int AcceptConnection(int);
int ReadMsg(int, char*, int);
int SendMsg(int, char*, int);

#endif
