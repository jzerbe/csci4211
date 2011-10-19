/**
 * sockcomm.c - helper functions for peer program
 */

#include "./sockcomm.h"

/**
 * convenience method for outputting error and exiting program
 * @param msg char* - the string to output as error message
 */
void ExitError(const char* theErrorMessage) {
    perror(theErrorMessage);
    exit(EXIT_FAILURE);
}

/**
 * returns the greater of a and b. if both are equivalent, a is returned
 * @see http://www.cplusplus.com/reference/algorithm/max/
 * 
 * @param a int
 * @param b int
 * @return int
 */
int MaximumHelper(int a, int b) {
    return (a < b) ? b : a;
}

/**
 * return file descriptor of socket after connecting to specified server and port
 * @param hostname char* - the hostname to connect to
 * @param port unsigned short - the port to connect to
 * @return int - the socket file descriptor, -1 on error
 */
int ConnectToServer(char* hostname, unsigned short port) {
    if ((hostname == NULL) || (port <= 0)) {
        return -1;
    }

    int sd;

    struct hostent* server_name = gethostbyname(hostname);
    if (server_name == NULL) {
        return -1;
    }

    struct sockaddr_in serv_sockaddr;
    serv_sockaddr.sin_family = AF_INET;
    bcopy((char *) server_name->h_addr, (char *) &serv_sockaddr.sin_addr.s_addr,
            server_name->h_length);
    serv_sockaddr.sin_port = htons(port);

    if (connect(sd, (struct sockaddr *) &serv_sockaddr, sizeof (serv_sockaddr)) < 0) {
        return -1;
    }

    return (sd);
}

/**
 * get information about socket bound locally
 * @param sockfd int - the socket file descriptor
 * @param hostname char* - pointer to buffer to fill of size at least MAXNAMELEN
 * @param port unsigned short* - pointer to fill with port number
 */
void LocalSocketInfo(int sockfd, char* hostname, unsigned short* port) {
    struct sockaddr_in local_addr;
    int local_addr_len = sizeof (local_addr);
    if (getsockname(sockfd, &local_addr, &local_addr_len) == 0) {
        if (port != NULL) *port = ntohs(local_addr.sin_port);

        if (hostname != NULL) {
            struct hostent* myhostent = gethostbyaddr(
                    (char *) &local_addr.sin_addr.s_addr,
                    sizeof (local_addr.sin_addr.s_addr), AF_INET);

            if (myhostent == NULL) { //unable to find hostname  - fail gracefully
                strncpy(hostname, inet_ntoa(local_addr.sin_addr), MAXNAMELEN);
            } else { //found hostname
                strncpy(hostname, myhostent->h_name, MAXNAMELEN);
            }

            hostname[(MAXNAMELEN - 1)] = '\0';
        }
    }
}

/**
 * get information about remote peer socket
 * @param sockfd int - the socket file descriptor
 * @param hostname char* - pointer to buffer to fill of size at least MAXNAMELEN
 * @param port unsigned short* - pointer to fill with port number
 */
void RemoteSocketInfo(int sockfd, char* hostname, unsigned short* port) {
    struct sockaddr_in remote_addr;
    int remote_addr_len = sizeof (remote_addr);
    if (getpeername(sockfd, (struct sockaddr*) &remote_addr, &remote_addr_len) == 0) {
        if (port != NULL) *port = ntohs(remote_addr.sin_port);

        if (hostname != NULL) {
            struct hostent* myhostent = gethostbyaddr(
                    (char *) &remote_addr.sin_addr.s_addr,
                    sizeof (remote_addr.sin_addr.s_addr), AF_INET);

            if (myhostent == NULL) { //unable to find hostname  - fail gracefully
                strncpy(hostname, inet_ntoa(remote_addr.sin_addr), MAXNAMELEN);
            } else { //found hostname
                strncpy(hostname, myhostent->h_name, MAXNAMELEN);
            }

            hostname[(MAXNAMELEN - 1)] = '\0';
        }
    }
}

/**
 * create a local listener socket bound to a certain port
 * @param port unsigned short - the port to bind on
 * @return int - the socket file descriptor, -1 on error
 */
int SocketInit(unsigned short port) {
    if (port <= 0) return -1;

    int sd = socket(AF_INET, SOCK_STREAM, 0);
    if (sd < 0) {
        return -1;
    }

    struct sockaddr_in serv_sockaddr;
    serv_sockaddr.sin_family = AF_INET;
    serv_sockaddr.sin_addr.s_addr = INADDR_ANY;
    serv_sockaddr.sin_port = htons(port);

    if (bind(sd, (struct sockaddr *) &serv_sockaddr, sizeof (serv_sockaddr)) < 0) {
        return -1;
    }
    if (listen(sd, LISTEN_BACKLOG) < 0) {
        return -1;
    }

    return (sd);
}

/**
 * wrapper function for the "accept" socket call
 * @param sockfd int - the socket file descriptor to be accepted
 * @return int - the accepted socket file descriptor, -1 on (accept) error
 */
int AcceptConnection(int sockfd) {
    if (sockfd < 0) {
        return -1;
    }

    struct sockaddr_in cli_sockaddr;
    socklen_t client_socklen = sizeof (cli_sockaddr);
    int newsockfd = accept(sockfd, (struct sockaddr *) &cli_sockaddr, &client_socklen);

    printf("Serving %s:%d\n", inet_ntoa(cli_sockaddr.sin_addr), ntohs(cli_sockaddr.sin_port));
    struct hostent *hp = gethostbyaddr((char *) &cli_sockaddr.sin_addr.s_addr, sizeof (cli_sockaddr.sin_addr.s_addr), AF_INET);
    printf("(Name is : %s)\n", hp->h_name);

    return newsockfd;
}

/**
 * read into a buffer from a certain file descriptor
 * @param fd int - the file descriptor to read from
 * @param buff char* - buffer to fill with the read message
 * @param size int - maximum number of bytes to read, 2 less buffer size
 * @return int - number of bytes read, -1 on error
 */
int ReadMsg(int fd, char* buff, int size) {
    if ((fd < 0) || (buff == NULL) || (size <= 0)) {
        return -1;
    }

    int nread = read(fd, buff, size);
    buff[nread] = '\0';

    return (nread);
}

/**
 * write to a certain file descriptor from a buffer
 * @param fd int - the file descriptor to write to
 * @param buff char* - buffer containing message to write
 * @param size int - maximum number of byte to write, 1 less than buffer size
 * @return int - number of bytes written, -1 on error
 */
int SendMsg(int fd, char* buff, int size) {
    if ((fd < 0) || (buff == NULL) || (size <= 0)) {
        return -1;
    }

    return (write(fd, buff, size));
}
