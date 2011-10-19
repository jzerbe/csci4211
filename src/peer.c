/**
 * csci4211 Fall 2011
 * Programming Assignment: Simple File Sharing System
 * 
 * peer.c - main peer program source file
 */

#include <arpa/inet.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include "sockcomm.h"

/**
 * join tree P2P network through other host
 * @param peerhost char* - the name of the bootstrap peer
 * @param peerport unsigned short - the port number on the bootstrap peer
 * @return int - the socket file descriptor, -1 on error
 */
int join(char *peerhost, unsigned short peerport) {
    int sd = ConnectToServer(peerhost, peerport);
    if (sd < 0) {
        return -1;
    }

    unsigned short aLocalPortNum;
    LocalSocketInfo(sd, NULL, &aLocalPortNum);

    printf("admin - connected to peer host on %s:%hu through %hu\n",
            peerhost, peerport, aLocalPortNum);

    return (sd);
}

int main(int argc, char *argv[]) {
    //check if we have adequate parameters
    if (argc != 2 && argc != 3) {
        printf("Usage: %s <directory pathname> [<peer name>]\n", argv[0]);
        exit(EXIT_SUCCESS);
    }


    //shared file list global variables
    char mySharePathCharArray[256];
    DIR *myShareDirPointer;
    struct dirent *myShareDirEntry;
    char filelist[2048]; //file names in share directory, separated by '\r\n'

    //grab shared file list
    strncpy(mySharePathCharArray, argv[1], 255);
    mySharePathCharArray[255] = '\0';
    if ((myShareDirPointer = opendir(mySharePathCharArray)) == NULL) {
        printf("unable to open share directory '%s'\n", mySharePathCharArray);
        exit(EXIT_FAILURE);
    } else { //directory was opened
        filelist[0] = '\0';
        while ((myShareDirEntry = readdir(myShareDirPointer)) != NULL) {
            if ((strncmp(myShareDirEntry->d_name, ".", 2) == 0) ||
                    (strncmp(myShareDirEntry->d_name, "..", 2) == 0))
                continue;
            strcat(filelist, myShareDirEntry->d_name); //TODO: use strncat
            strcat(filelist, "\r\n"); //TODO: use strncat
        }
        closedir(myShareDirPointer);
    }


    //start local server
    int myLocalJoinServerSocket = SocketInit(JOIN_PORT);
    if (myLocalJoinServerSocket < 0) {
        perror("unable to start local join server socket");
        exit(EXIT_FAILURE);
    }
    //grab local address information
    char aLocalHostName[MAXNAMELEN];
    LocalSocketInfo(myLocalJoinServerSocket, aLocalHostName, NULL);
    printf("admin - started join server on %s:%hu\n", aLocalHostName, JOIN_PORT);


    //join network via given bootstrap peer
    int myBootStrapPeerSock = -1;
    if (argc == 3) {
        // connect to a known peer in the system
        myBootStrapPeerSock = join(argv[2], JOIN_PORT);
        if (myBootStrapPeerSock < 0) {
            perror("unable to connect to bootstrap peer");
            exit(EXIT_FAILURE);
        }
    }


    //select loop variables
    fd_set myMasterFileDescReadSet; //automatically updated each select loop - DO NOT TOUCH
    fd_set myActiveFileDesc; //track active descriptors - set to update in logic
    FD_ZERO(myMasterFileDescReadSet);
    FD_ZERO(myActiveFileDesc);
    FD_SET(myLocalJoinServerSocket, &myActiveFileDesc);
    if (myBootStrapPeerSock > -1) { //only add the bootstrap socket if used
        FD_SET(myBootStrapPeerSock, &myActiveFileDesc);
    }
    //set the maximum number of sockets to select
    int myActiveFileDescMax = MaximumHelper(myLocalJoinServerSocket, myBootStrapPeerSock);

    char szBuffer[2048];
    int hListenSock;
    int hDataSock;

    //more on select loops --> http://www.tenouk.com/Module41.html
    while (1) {
        int frsock = -1;
        myMasterFileDescReadSet = myActiveFileDesc;

        /* watch for stdin, myLocalJoinServerSocket and other peer sockets */
        if (select(myActiveFileDescMax + 1, &myMasterFileDescReadSet, NULL, NULL, NULL) < 0) {
            perror("select failure");
            exit(EXIT_FAILURE);
        }

        /* check lookup requests from neighboring peers */
        for (frsock = 3; frsock <= myActiveFileDescMax; frsock++) {
            //frsock starts from 3: stdin = 0, stdout = 1, stderr = 2
            if (frsock == myLocalJoinServerSocket) continue;

            if (FD_ISSET(frsock, &myMasterFileDescReadSet)) {
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
        if (FD_ISSET(0, &myMasterFileDescReadSet)) {
            if (!fgets(szBuffer, MAXMSGLEN, stdin)) exit(EXIT_FAILURE);

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

        /* join request on myLocalJoinServerSocket from a new peer */
        if (FD_ISSET(myLocalJoinServerSocket, &myMasterFileDescReadSet)) {
            int newsocketfd = AcceptConnection(myLocalJoinServerSocket);
            if (newsocketfd < 0) {
                perror("AcceptConnection failure");
            } else {
                FD_SET(newsocketfd, &myActiveFileDesc);
                myActiveFileDescMax = MaximumHelper(myActiveFileDescMax, newsocketfd);

                char aRemoteHostName[MAXNAMELEN];
                unsigned short aRemotePortNum;
                RemoteSocketInfo(newsocketfd, aRemoteHostName, &aRemotePortNum);
                printf("admin - join from %s:%hu\n", aRemoteHostName, aRemotePortNum);
            }
        }
    }
}
