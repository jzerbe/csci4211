/*
        csci4211 Fall 2011
        Programming Assignment: Simple File Sharing System
 */
#define JOIN_PORT 8000
#define MAXMSGLEN  1024
#define MAXNAMELEN 128

#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <time.h>
#include <errno.h>
#include <dirent.h>
#include "sockcomm.h"

/*
  User by a new peer host to join the system
  throught host 'peerhost'.
 */
int join(char *peerhost, ushort peerport) {
    int sd;
    struct sockaddr_in myaddr;

    sd = ConnectToServer(peerhost, peerport);

    /* 
       FILL HERE:
       use getsockname() to find the port number
     */

    printf("admin: connected to peer host on '%s' at '%hu' thru '%hu'\n",
            peerhost, peerport, ntohs(myaddr.sin_port));

    return (sd);
}

int main(int argc, char *argv[]) {
    int hJoinSock;
    int hListenSock;
    int hDataSock;
    int sock;
    int livesdmax; /* maximum active descriptor */
    fd_set readset; /* used for select() */
    fd_set livesdset; /* used to keep track of active descriptors */
    char szDir[256];
    DIR *dp;
    struct dirent *dirp;
    char filelist[2048]; /* contains all file names in the shared directory, separated by '\r\n' */
    char szBuffer[2048];
    struct hostent *myhostent;

    if (argc != 2 && argc != 3) {
        printf("Usage: %s <directory pathname> [<peer name>]\n", argv[0]);
        exit(0);
    }

    /* argv[1] contains the pathname of the directory that is shared */
    strcpy(szDir, argv[1]);
    if ((dp = opendir(szDir)) == NULL) {
        printf("Can not open directory '%s'\n", szDir);
        exit(0);
    } else {
        filelist[0] = '\0';
        while ((dirp = readdir(dp)) != NULL) {
            if ((strcmp(dirp->d_name, ".") == 0) || (strcmp(dirp->d_name, "..") == 0)) continue;
            strcat(filelist, dirp->d_name);
            strcat(filelist, "\r\n");
        }
        closedir(dp);
    }

    /* start accepting join request of new peers */
    if ((hJoinSock = SocketInit(JOIN_PORT)) == -1) {
        perror("SocketInit");
        exit(1);
    }

    /* figure out the name of this host */
    /*
      FILL HERE:
      Use gethostname() and gethostbyname() to figure the full name of the host
     */

    printf("admin: started server on '%s' at '%hu'\n", myhostent->h_name, JOIN_PORT);


    /* if address is specified, join the system from the specified node */
    if (argc > 2) {
        // connect to a known peer in the system
        sock = join(argv[2], JOIN_PORT);
        if (sock == -1) exit(1);
    }

    /*
      FILL HERE:
      Prepare parameters for the select()
      Note: 'sock' is not set for the first member since it doesn't join others
     */

    while (1) {
        int frsock;

        memcpy(&readset, &livesdset, sizeof (fd_set));

        /* watch for stdin, hJoinSock and other peer sockets */
        if (select(livesdmax + 1, &readset, NULL, NULL, NULL) == -1) {
            perror("select");
            exit(1);
        }

        /* check lookup requests from neighboring peers */
        for (frsock = 3; frsock <= livesdmax; frsock++) {
            /* 
               frsock starts from 3 since descriptor 0, 1 and 2 are
               stdin, stdout and stderr
             */
            if (frsock == hJoinSock) continue;

            if (FD_ISSET(frsock, &readset)) {
                /*
                  FILL HERE:
                  1. Read message from socket 'frsock';
                  2. If message size is 0, the peer host has disconnected. Print out message
                     like "admin: disconnected from 'venus.cs.umn.edu(42453)'"
                  3. If message size is >0, inspect whehter it is a 'GET' message
                  3.1. Extract file name, IP address and port number in the 'GET' message
                  3.2. Do local lookup on 'filelist' to check whether requested file is in 
                       this peerhost
                  3.3  If requested file presents, make connection to originating host and send data
                  3.4  If requested file is not here, forward 'GET' message to all neighbors except
                       the incoming one
                 */
            }
        }

        /* input message from stdin */
        if (FD_ISSET(0, &readset)) {
            if (!fgets(szBuffer, MAXMSGLEN, stdin)) exit(0);

            /*
              FILL HERE:
              1. Inspect whether input command is a valid 'get' command with file name
              2. create new listen socket for future data channel setup
              3. User fork() to generate a child process
              4. In the child process, use select() to monitor listen socket. Set a timout
                 period for the select(). If no connection after timeout, close the listen
                 socket. If select() returns with connection, use AcceptConnection() on listen
                 socket to setup data connection. Then, download the file from remote peer.
              5. In the parent process, construct "GET" message and send it to all neighbor peers.
             */
        }

        /* join request on hJoinSock from a new peer */
        if (FD_ISSET(hJoinSock, &readset)) {
            /*
              FILL HERE:
              1. Accept connection request for new joining peer host.
              2. Update 'livesdset' and 'livesdmax'
              3. print out message like "admin: join from 'venus.cs.umn.edu' at '34234'"
             */
        }
    }
}
