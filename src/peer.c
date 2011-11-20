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
#include <limits.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include "./sockcomm.h"
#include "./argparse.h"

#define true 1
#define false 0
#define bool char

#ifndef size_t
#define size_t unsigned int
#endif

/**
 * utility function for replacing chars
 * @param buff char* - the read/write char array
 * @param oldChar char - the char to search for
 * @param newChar char - the char to replace with
 * @param maxSearchLength unsigned int - the maximum number of elements to traverse
 */
void replaceChars(char* buff, char oldChar, char newChar, unsigned int maxSearchLength) {
    if ((buff == NULL) || (maxSearchLength < 0) || (maxSearchLength > UINT_MAX)) return;

    unsigned int i = 0;
    for (i = 0; i < maxSearchLength; i++) {
        if (buff[i] == oldChar) buff[i] = newChar;
    }
}

/**
 * generic signal handler
 * @param theSignalNumber int - the signal type number
 */
void signalHandler(int theSignalNumber) {
    printf("\nSignal %d caught\n", theSignalNumber);
    if (theSignalNumber == 2) { //SIGINT
        exit(0);
    }
}

/**
 * join tree P2P network through other host
 * @param peerhost char* - the name of the bootstrap peer
 * @param peerport int - the port number on the bootstrap peer
 * @return int - the socket file descriptor, -1 on error
 */
int join(char *peerhost, int peerport) {
    int sd = ConnectToServer(peerhost, peerport);
    if (sd < 0) {
        return -1;
    }

    int aLocalPortNum;
    LocalSocketInfo(sd, NULL, &aLocalPortNum);

    printf("admin - connected to peer host on %s:%hu through %hu\n",
            peerhost, peerport, aLocalPortNum);

    return (sd);
}

/**
 * index the directory and put a '\n' delimited list of files in the buffer
 * @param theSharePathStr char* - the share path string
 * @param theFileListBuffer char* - the string buffer to fill
 * @param theBufSize size_t - how big is the buffer?
 * @return int - return -1 on error
 */
int indexShareDir(char* theSharePathStr, char* theFileListBuffer, size_t theBufSize) {
    //shared file list global variables
    char aSharePathCharArray[256];
    DIR *aShareDirPointer;
    struct dirent *aShareDirEntry;

    //grab shared file list
    strncpy(aSharePathCharArray, theSharePathStr, 255);
    aSharePathCharArray[255] = '\0';
    if ((aShareDirPointer = opendir(aSharePathCharArray)) == NULL) {
#ifdef DEBUG
        printf("main: opendir failure - unable to open share directory '%s'\n", aSharePathCharArray);
#endif
        return -1;
    } else { //directory was opened
        memset(theFileListBuffer, '\0', theBufSize);
        while ((aShareDirEntry = readdir(aShareDirPointer)) != NULL) {
            if ((strncmp(aShareDirEntry->d_name, ".", 2) == 0) ||
                    (strncmp(aShareDirEntry->d_name, "..", 2) == 0))
                continue;
            strcat(theFileListBuffer, aShareDirEntry->d_name); //TODO: use strncat
            strcat(theFileListBuffer, "\n"); //TODO: use strncat
        }
        closedir(aShareDirPointer);
    }

    return 0;
}

int main(int argc, char *argv[]) {
    //install SIGINT signal handler
    struct sigaction my_sigaction_sigint;
    my_sigaction_sigint.sa_handler = signalHandler;
    sigemptyset(&my_sigaction_sigint.sa_mask);
    my_sigaction_sigint.sa_flags = 0;
    sigaction(SIGINT, &my_sigaction_sigint, NULL);

    //check if we have adequate parameters
    if (argc != 2 && argc != 3) {
        printf("Usage: %s <directory pathname> [<peer name>]\n", argv[0]);
        exit(EXIT_SUCCESS);
    }


    //index the shared directory
    char myFileIndexString[2048];
    if (indexShareDir(argv[1], myFileIndexString, 2048) != 0) {
        perror("main: indexShareDir failure");
        exit(EXIT_FAILURE);
    }

    //start local join server listener on free port
    int myLocalJoinServerSocket = SocketInit(JOIN_PORT);
    if (myLocalJoinServerSocket < 0) {
        perror("main: SocketInit failure - unable to create join listener socket");
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
            perror("main: join failure - unable to connect to bootstrap peer");
            exit(EXIT_FAILURE);
        }
    }


    //select loop variables
    fd_set myMasterFileDescReadSet; //automatically updated each select loop - DO NOT TOUCH
    fd_set myActiveFileDesc; //track active descriptors - set to update in logic
    FD_ZERO(&myMasterFileDescReadSet);
    FD_ZERO(&myActiveFileDesc);
    FD_SET(STDIN_FILENO, &myActiveFileDesc); //we do want to read from STDIN
    FD_SET(myLocalJoinServerSocket, &myActiveFileDesc);
    if (myBootStrapPeerSock > -1) { //only add the bootstrap socket if used
        FD_SET(myBootStrapPeerSock, &myActiveFileDesc);
    }
    //set the maximum number of sockets to select
    int myActiveFileDescMax = MaximumHelper(myLocalJoinServerSocket, myBootStrapPeerSock);

    //select loop
    while (1) {
        int frsock = -1;
        myMasterFileDescReadSet = myActiveFileDesc;

        /* watch for stdin, myLocalJoinServerSocket and other peer sockets */
        if (select(myActiveFileDescMax + 1, &myMasterFileDescReadSet, NULL, NULL, NULL) < 0) {
            perror("main: select failure");
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
                  3. If message size is >0, inspect whether it is a 'GET' message
                  3.1. Extract file name, IP address and port number in the 'GET' message
                  3.2. Do local lookup on 'filelist' to check whether requested file is in
                       this peerhost
                  3.3  If requested file presents, make connection to originating host and send data
                  3.4  If requested file is not here, forward 'GET' message to all neighbors except
                       the incoming one
                 */
                char aBuff[MAXMSGLEN];
                int aByteReadCount = ReadMsg(frsock, aBuff, MAXMSGLEN);
                if (aByteReadCount <= 0) { //remote end hung up or error
                    //get socket info before closing out connection on our end
                    char aRemoteHostName[MAXNAMELEN];
                    int aRemotePortNum;
                    RemoteSocketInfo(frsock, aRemoteHostName, &aRemotePortNum);

                    //close out connection
                    close(frsock);
                    FD_CLR(frsock, &myActiveFileDesc);
                    printf("admin - disconnected %s:%hu\n", aRemoteHostName, aRemotePortNum);
                }
            }
        }

        /* input message from stdin */
        if (FD_ISSET(STDIN_FILENO, &myMasterFileDescReadSet)) {
            char aStdInBuffer[2048];
            if (!fgets(aStdInBuffer, MAXMSGLEN, stdin)) exit(EXIT_FAILURE);

#ifdef DEBUG
            printf("STDIN> %s\n", aStdInBuffer);
#endif

            //make sure the input in properly formatted
            replaceChars(aStdInBuffer, '\r', '\0', MAXMSGLEN);
            replaceChars(aStdInBuffer, '\n', '\0', MAXMSGLEN);

            //what operation are we doing?
            if (strncmp(aStdInBuffer, "quit", 4) == 0) {
                exit(0);
            } else if (strncmp(aStdInBuffer, "list", 4) == 0) {
                printf("\nShared Files:\n%s\n", myFileIndexString);
            } else if (strncmp(aStdInBuffer, "get ", 4) == 0) {
                int argc;
                char** argv;
                if (createArgcArgv(aStdInBuffer, &argc, &argv) == 0) {
                    if (argc == 2) {
                        //
                    }
                }
            }

            /*
              FILL HERE:
              1. Inspect whether input command is a valid 'get' command with file name
              2. Create new listen socket for future data channel setup
              3. Use fork() to generate a child process
              4. In the child process, use select() to monitor listen socket. Set a timeout
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
                perror("main: AcceptConnection failure - unable to accept new peer");
            } else {
                FD_SET(newsocketfd, &myActiveFileDesc);
                myActiveFileDescMax = MaximumHelper(myActiveFileDescMax, newsocketfd);

                char aRemoteHostName[MAXNAMELEN];
                int aRemotePortNum;
                RemoteSocketInfo(newsocketfd, aRemoteHostName, &aRemotePortNum);
                printf("admin - join from %s:%hu\n", aRemoteHostName, aRemotePortNum);
            }
        }
    }

    return 0;
}
