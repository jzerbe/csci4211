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

const int LISTEN_BACKLOG = 10;

#define JOIN_PORT 8000
#define MAXMSGLEN  1024
#define MAXNAMELEN 128

void ExitError(const char*);
int MaximumHelper(int, int);
int ConnectToServer(char*, unsigned short);
void LocalSocketInfo(int, char*, unsigned short*);
void RemoteSocketInfo(int, char*, unsigned short*);
int SocketInit(unsigned short);
int AcceptConnection(int);
int ReadMsg(int, char*, int);
int SendMsg(int, char*, int);

#endif
