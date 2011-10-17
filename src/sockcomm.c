/**
 * sockcomm.c - helper functions for peer program
 */

#include "./sockcomm.h"

/**
 * convenience method for outputting error and exiting program
 * 
 * @param msg char* - the string to output as error message
 */
void ExitError(const char* theErrorMessage) {
    perror(theErrorMessage);
    exit(1);
}

/**
 * return the file descriptor of the socket after connecting
 * to the specified server and port
 * 
 * @param hostname char* - the hostname to connect to
 * @param port int - the port to connect to
 * @return int - the socket file descriptor, -1 on error
 */
int ConnectToServer(char* hostname, int port) {
    if ((hostname == NULL) || (port <= 0)) return -1;

    int sd;

    struct hostent* server_name = gethostbyname(hostname);
    if (server_name == NULL) return -1;

    struct sockaddr_in serv_sockaddr;
    serv_sockaddr.sin_family = AF_INET;
    bcopy((char *) server_name->h_addr, (char *) &serv_sockaddr.sin_addr.s_addr,
            server_name->h_length);
    serv_sockaddr.sin_port = htons(port);

    if (connect(sd, (struct sockaddr *) &serv_sockaddr, sizeof (serv_sockaddr)) < 0)
        return -1;

    return (sd);
}

/**
 * create a local listener socket bound to a certain port
 * 
 * @param port int - the port to bind on
 * @return int - the socket file descriptor, -1 on error
 */
int SocketInit(int port) {
    if (port <= 0) return -1;

    int sd = socket(AF_INET, SOCK_STREAM, 0);
    if (sd < 0) return -1;

    struct sockaddr_in serv_sockaddr;
    serv_sockaddr.sin_family = AF_INET;
    serv_sockaddr.sin_addr.s_addr = INADDR_ANY;
    serv_sockaddr.sin_port = htons(port);

    if (bind(sd, (struct sockaddr *) &serv_sockaddr, sizeof (serv_sockaddr)) < 0)
        return -1;
    if (listen(sd, LISTEN_BACKLOG) < 0)
        return -1;

    return (sd);
}

/**
 * wrapper function for the "accept" socket call
 * 
 * @param sockfd int - the socket file descriptor to be accepted
 * @return int - the accepted socket file descriptor, -1 on (accept) error
 */
int AcceptConnection(int sockfd) {
    if (sockfd < 0) return -1;

    struct sockaddr_in cli_sockaddr;
    socklen_t client_socklen = sizeof (cli_sockaddr);
    int newsockfd = accept(sockfd, (struct sockaddr *) &cli_sockaddr, &client_socklen);

    return newsockfd;
}

/**
 * read into a buffer from a certain file descriptor
 * 
 * @param fd int - the file descriptor to read from
 * @param buff char* - buffer to fill with the read message
 * @param size int - maximum number of bytes to read, 2 less buffer size
 * @return int - number of bytes read, -1 on error
 */
int ReadMsg(int fd, char* buff, int size) {
    if ((fd < 0) || (buff == NULL) || (size <= 0)) return -1;

    int nread = read(fd, buff, size);
    buff[nread] = '\0';

    return (nread);
}

/**
 * write to a certain file descriptor from a buffer
 * 
 * @param fd int - the file descriptor to write to
 * @param buff char* - buffer containing message to write
 * @param size int - maximum number of byte to write, 1 less than buffer size
 * @return int - number of bytes written, -1 on error
 */
int SendMsg(int fd, char* buff, int size) {
    if ((fd < 0) || (buff == NULL) || (size <= 0)) return -1;

    return (write(fd, buff, size));
}
