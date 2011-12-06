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
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include "./sockcomm.h"

#ifndef size_t
#define size_t unsigned int
#endif

static unsigned int myDataTransferPortNumber = 36911;
static int myLocalJoinServerSocket;

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
 * check for a certain filename if it is in the shared index
 * @param theFileIndexStr char* - the "\n" delimited list of shared files
 * @param theFileName char* - the filename to search for in the index
 * @return bool - do we have file in index?
 */
bool fileExistsInIndex(char* theFileIndexStr, char* theFileName) {
    if ((theFileIndexStr == NULL) || (theFileName == NULL)) return false;

    char aWorkingFileIndexStr[strlen(theFileIndexStr)];
    strcpy(aWorkingFileIndexStr, theFileIndexStr); //create temp, as it gets broken

#ifdef DEBUG
    printf("fileExistsInIndex: theFileIndexStr = '%s'\n", theFileIndexStr);
#endif

    char* saveptr_in;
    char* pch = strtok_r(aWorkingFileIndexStr, "\n", &saveptr_in);

#ifdef DEBUG
    printf("fileExistsInIndex: theFileIndexStr = '%s'\n", theFileIndexStr);
#endif

    while (pch != NULL) {
        if (strncmp(pch, theFileName, FILENAME_MAX) == 0) return true;
        pch = strtok_r(NULL, "\n", &saveptr_in);
    }

#ifdef DEBUG
    printf("fileExistsInIndex: theFileIndexStr = '%s'\n", theFileIndexStr);
#endif

    return false;
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
 * generic signal handler
 * @param theSignalNumber int - the signal type number
 */
void signalHandler(int theSignalNumber) {
    printf("\nSignal %d caught\n", theSignalNumber);
    if (theSignalNumber == 2) { //SIGINT
        close(myLocalJoinServerSocket);
        exit(0);
    }
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

    //check to make sure shared dir has trailing slash
    if (argv[1][(strnlen(argv[1], FILENAME_MAX) - 1)] != '/') {
        perror("main: shared directory does not have trailing slash");
        exit(EXIT_FAILURE);
    }

    //index the shared directory
    char myFileIndexString[(FILENAME_MAX * 20)];
    if (indexShareDir(argv[1], myFileIndexString, (FILENAME_MAX * 20)) != 0) {
        perror("main: indexShareDir failure");
        exit(EXIT_FAILURE);
    }
#ifdef DEBUG
    printf("main: myFileIndexString = '%s'\n", myFileIndexString);
#endif

    //start local join server listener on free port
    myLocalJoinServerSocket = SocketInit(JOIN_PORT);
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
            perror("main: master select failure");
            exit(EXIT_FAILURE);
        }

        /* check lookup requests from neighboring peers */
        for (frsock = 3; frsock <= myActiveFileDescMax; frsock++) {
            //frsock starts from 3: stdin = 0, stdout = 1, stderr = 2
            if (frsock == myLocalJoinServerSocket) continue;

            if (FD_ISSET(frsock, &myMasterFileDescReadSet)) {
                char aBuff[MAXMSGLEN];
                int aByteReadCount = ReadMsg(frsock, aBuff, MAXMSGLEN);
                if (aByteReadCount <= 0) { //remote end hung up or error
                    //get socket info before closing out connection on our end
                    char aRemoteHostName[MAXNAMELEN];
                    int aRemotePortNum;
                    RemoteSocketInfo(frsock, aRemoteHostName, &aRemotePortNum, true);

                    //close out connection
                    close(frsock);
                    FD_CLR(frsock, &myActiveFileDesc);
                    printf("admin - disconnected %s:%hu\n", aRemoteHostName, aRemotePortNum);
                } else {
#ifdef DEBUG
                    printf("incoming aBuff = '%s'\n", aBuff);
#endif
                    if (strncmp(aBuff, "get", 3) == 0) {
                        char aRequestFileName[MAXMSGLEN];
                        char aRequestSourceAddress[MAXMSGLEN];
                        char aRequestPortNumber[MAXMSGLEN];

                        int get_argc = 0;
                        char* saveptr;
                        char* pch = strtok_r(aBuff, " ", &saveptr);
                        while (pch != NULL) {
#ifdef DEBUG
                            printf("incoming aBuff - pch = '%s'\n", pch);
#endif
                            if (strncmp(pch, "", 1) != 0) { //valid - get [filename] [src address] [data port]
                                if (get_argc == 1) {
                                    strcpy(aRequestFileName, pch);
                                } else if (get_argc == 2) {
                                    if (strncmp(pch, "0.0.0.0", 7) == 0) {
                                        RemoteSocketInfo(frsock, aRequestSourceAddress, NULL, true);
                                    } else {
                                        strcpy(aRequestSourceAddress, pch);
                                    }
                                } else if (get_argc == 3) {
                                    strcpy(aRequestPortNumber, pch);
                                }
                                get_argc++;
                            }

                            pch = strtok_r(NULL, " ", &saveptr);
                        }
#ifdef DEBUG
                        printf("%i arguments in get request\n", get_argc);
#endif
                        if (get_argc == 4) {
                            //re-index share directory, to have latest listings
                            if (indexShareDir(argv[1], myFileIndexString, (FILENAME_MAX * 20)) != 0) {
                                perror("main: (re)indexShareDir failure");
                            }
#ifdef DEBUG
                            printf("main: myFileIndexString = '%s'\n", myFileIndexString);
#endif
                            if (fileExistsInIndex(myFileIndexString, aRequestFileName)) { //return data to requester
#ifdef DEBUG
                                printf("main: myFileIndexString = '%s'\n", myFileIndexString);
#endif
                                char aLocalFilePathStr[FILENAME_MAX];
                                memset(aLocalFilePathStr, '\0', FILENAME_MAX);
                                strcpy(aLocalFilePathStr, argv[1]);
                                strcat(aLocalFilePathStr, aRequestFileName);

                                int aLocalFileFd = open(aLocalFilePathStr, O_RDONLY);
#ifdef DEBUG
                                printf("will now use local file descriptor #%i\n", aLocalFileFd);
#endif
                                char aTransferBuffer[MAXMSGLEN];
                                int numbytes;
                                if (aLocalFileFd >= 0) {
                                    int aPort = atoi(aRequestPortNumber);
#ifdef DEBUG
                                    printf("connect to = '%s:%hu' in get request\n", aRequestSourceAddress, aPort);
#endif
                                    int aTransferFd = ConnectToServer(aRequestSourceAddress, aPort);
                                    if (aTransferFd >= 0) { //okay to start transfer
                                        while ((numbytes = ReadMsg(aLocalFileFd, aTransferBuffer, MAXMSGLEN - 1)) > 0) {
                                            if (SendMsg(aTransferFd, aTransferBuffer, numbytes) < 0) {
#ifdef DEBUG
                                                perror("main: SendMsg failure - return data stream");
#endif
                                            }
                                        }
                                        close(aTransferFd);
                                    }
                                }
                                close(aLocalFileFd);
                            } else { //forward request to all peers except incoming and self
                                char aForwardBuff[MAXMSGLEN];
                                memset(aForwardBuff, '\0', MAXMSGLEN);
                                strncpy(aForwardBuff, "get ", MAXMSGLEN);
                                strncat(aForwardBuff, aRequestFileName, MAXMSGLEN);
                                strncat(aForwardBuff, " ", 1);
                                strncat(aForwardBuff, aRequestSourceAddress, MAXMSGLEN);
                                strncat(aForwardBuff, " ", 1);
                                strncat(aForwardBuff, aRequestPortNumber, MAXMSGLEN);
                                int i;
                                for (i = 3; i <= myActiveFileDescMax; i++) {
                                    if (FD_ISSET(i, &myActiveFileDesc)) {
                                        if ((i != frsock) && (i != myLocalJoinServerSocket)) {
                                            if (SendMsg(i, aForwardBuff, MAXMSGLEN) < 0) {
#ifdef DEBUG
                                                perror("main: SendMsg failure - forwarding of get");
#endif
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
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
                //re-index share directory, to have latest listings
                if (indexShareDir(argv[1], myFileIndexString, (FILENAME_MAX * 20)) != 0) {
                    perror("main: (re)indexShareDir failure");
                }
                printf("\nShared Files:\n%s\n", myFileIndexString);
            } else if (strncmp(aStdInBuffer, "get", 3) == 0) {
                char aRequestFileName[MAXMSGLEN];

                int get_argc = 0;
                char* saveptr;
                char* pch = strtok_r(aStdInBuffer, " ", &saveptr);
                while (pch != NULL) {
#ifdef DEBUG
                    printf("aStdInBuffer - pch = '%s'\n", pch);
#endif
                    if (strncmp(pch, "", 1) != 0) { //valid - get [filename]
                        if (get_argc == 1) {
                            strcpy(aRequestFileName, pch);
                        }
                        get_argc++;
                    }

                    pch = strtok_r(NULL, " ", &saveptr);
                }
#ifdef DEBUG
                printf("%i arguments in get request\n", get_argc);
#endif
                if (get_argc == 2) {
                    //re-index share directory, to have latest listings
                    if (indexShareDir(argv[1], myFileIndexString, (FILENAME_MAX * 20)) != 0) {
                        perror("main: (re)indexShareDir failure");
                    }
#ifdef DEBUG
                    printf("main: myFileIndexString = '%s'\n", myFileIndexString);
#endif
                    if (!fileExistsInIndex(myFileIndexString, aRequestFileName)) {
#ifdef DEBUG
                        printf("main: myFileIndexString = '%s'\n", myFileIndexString);
#endif
                        //need to get file, create data transfer socket
                        myDataTransferPortNumber++;
                        int myDataTransferSocket;
                        if ((myDataTransferSocket = SocketInit(myDataTransferPortNumber)) >= 0) {
                            //fork off new process for data transfer
                            pid_t pID = fork();
                            if (pID == 0) { //fork child
                                fd_set myDataTransferDesc;
                                FD_ZERO(&myDataTransferDesc);
                                FD_SET(myDataTransferSocket, &myDataTransferDesc);
                                int myDataTransferDescMax = myDataTransferSocket;
                                struct timeval timeout;
                                timeout.tv_sec = 5;
                                timeout.tv_usec = 0;
                                int n = select(myDataTransferDescMax + 1, &myDataTransferDesc, NULL, NULL, &timeout);
                                if (n < 0) { //error
                                    perror("main: data transfer select failure");
                                    exit(EXIT_FAILURE);
                                } else if (n == 0) { //timeout
                                    printf("admin - the requested file does not exist in the p2p system\n");
                                    close(myDataTransferSocket);
                                } else { //have data
#ifdef DEBUG
                                    printf("received data on transfer socket\n");
#endif
                                    if (FD_ISSET(myDataTransferSocket, &myDataTransferDesc)) {
                                        char aLocalFilePathStr[FILENAME_MAX];
                                        memset(aLocalFilePathStr, '\0', FILENAME_MAX);
                                        strcpy(aLocalFilePathStr, argv[1]);
                                        strcat(aLocalFilePathStr, aRequestFileName);

                                        int aLocalFileFd = open(aLocalFilePathStr, (O_CREAT | O_RDWR), S_IRWXU);
                                        char aTransferBuffer[MAXMSGLEN];
                                        int numbytes;
                                        if (aLocalFileFd >= 0) {
                                            int aTransferFd = AcceptConnection(myDataTransferSocket);
                                            if (aTransferFd >= 0) { //okay to start transfer
#ifdef DEBUG
                                                printf("starting data transfer ...\n");
#endif
                                                while ((numbytes = ReadMsg(aTransferFd, aTransferBuffer, MAXMSGLEN - 1)) > 0) {
#ifdef DEBUG
                                                    printf("aTransferBuffer = '%s'\n", aTransferBuffer);
#endif
                                                    if (SendMsg(aLocalFileFd, aTransferBuffer, numbytes) < 0) {
#ifdef DEBUG
                                                        perror("main: SendMsg failure - write data to file");
#endif
                                                    }
                                                }
                                                printf("admin - the requested file has been downloaded\n");
                                                close(aTransferFd);
                                            }
                                            close(aLocalFileFd);
                                        }
                                    }
                                }
                                exit(EXIT_SUCCESS); //close out child process
                            } else if (pID > 0) { //fork parent
                                //valid - get [filename] [src address] [data port]
                                char aBuff[MAXMSGLEN];
                                if (sprintf(aBuff, "get %s 0.0.0.0 %i", aRequestFileName, myDataTransferPortNumber) > 0) {
#ifdef DEBUG
                                    printf("message to send = '%s'\n", aBuff);
#endif
                                    int i; //send to all peers
                                    for (i = 3; i <= myActiveFileDescMax; i++) {
                                        if (FD_ISSET(i, &myActiveFileDesc)) { //TODO: messages not being sent to any peers
                                            if ((i != myLocalJoinServerSocket) && (i != myDataTransferSocket)) {
                                                if (SendMsg(i, aBuff, MAXMSGLEN) < 0) {
#ifdef DEBUG
                                                    perror("main: SendMsg failure - broadcast of get");
#endif
                                                } else {
#ifdef DEBUG
                                                    printf("send message on socket #%i\n", i);
#endif
                                                }
                                            }
                                        }
                                    }
#ifdef DEBUG
                                    printf("finished sending messages to %i peers\n", (i - 4));
#endif
                                }
                            } else { //fork failure
                                perror("main: fork failure - failed to start data receiver process");
                            }
                        } else {
                            perror("main: SocketInit failure - failed to create data transfer socket");
                        }
                    }
                }
            }
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
                RemoteSocketInfo(newsocketfd, aRemoteHostName, &aRemotePortNum, true);
                printf("admin - join from %s:%hu\n", aRemoteHostName, aRemotePortNum);
            }
        }
    }

    return 0;
}
