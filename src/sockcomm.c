/*********************************************
 *
 * File name	    : sockcomm.c 
 * Description    : socket communication program
 *
 **********************************************/

#include "sockcomm.h"
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

/* Function: ConnectToServer
 * 1. Connect to specified server and port
 * 2. Return socket
 */
int ConnectToServer(char* hostname, int port) {
    int sd;

    /*
      FILL HERE:
      connect to server, and return the socekt. The parameter 'hostname' is
      the host name such as 'venus'. Use gethostbyname() to get host address info.
     */

    return (sd);
}

/* Function: SocketInit
 * 1. Create a socket, bind to local IP and 'port', and listen to the socket
 * 2. Return the socket
 */
int SocketInit(int port) {
    int sd;

    /*
      FILL HERE:
     */

    return (sd);
}

/* Function: AcceptCall
 * 1. Accept a connection request
 * 2. return the accepted socket
 */
int AcceptConnection(int sockfd) {
    int newsock;

    /*
      FILL HERE:
      use accept() on 'sockfd' to accept connection
     */

    return newsock;
}

/* Function: ReadMsg
 * Read a message
 */
int ReadMsg(int fd, char* buff, int size) {
    int nread = read(fd, buff, size);
    buff[nread] = '\0';
    return (nread);
}

/* Function: SendMsg
 * receive a message
 */
int SendMsg(int fd, char* buff, int size) {
    return (write(fd, buff, size));
}

