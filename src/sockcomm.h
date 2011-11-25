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

#define JOIN_PORT 8831
#define MAXMSGLEN  1024
#define MAXNAMELEN 128

#ifndef bool
#define true 1
#define false 0
#define bool char
#endif

void ExitError(const char*);
int MaximumHelper(int, int);
int ConnectToServer(char*, int);
void RemoteSocketInfo(int, char*, int*, bool);
int SocketInit(int);
void LocalSocketInfo(int, char*, int*);
int AcceptConnection(int);
int ReadMsg(int, char*, int);
int SendMsg(int, char*, int);

#endif
