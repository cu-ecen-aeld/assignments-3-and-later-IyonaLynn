/***********************************************************************
* @file  aesdsocket.c
* @version 0
* @brief  Implementation of socket
*
* @author Iyona Lynn Noronha, iyonalynn.noronha@Colorado.edu
*
* @institution University of Colorado Boulder (UCB)
* @course   ECEN 5713 - Advanced Embedded Software Development
* @instructor Dan Walkes
*
* Revision history:
*   0 Initial release.
*
*Ref:
* 1. Lecture Videos
* 2. https://beej.us/guide/bgnet/html/
* 3. Chatgpt: Prompt: Socket creation and listening
*/

#define _POSIX_C_SOURCE 200112L  // Enable POSIX features

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <syslog.h>
#include <fcntl.h>
#include <stdbool.h>
#include <errno.h>
#include <netdb.h>

#define PORT "9000"    // Port to listen on
#define BACKLOG 10     // Max pending connections
#define BUF_SIZE 1024  // Buffer size for receiving data
#define DATA_FILE "/var/tmp/aesdsocketdata"

volatile sig_atomic_t stop_flag = 0;

int sockfd = -1;  // Socket file descriptor
FILE *tmp_file = NULL;

// Function to handle cleanup on exit
void cleanup() {
    if (sockfd != -1) {
        close(sockfd);
        sockfd = -1;
    }
    if (tmp_file != NULL) {
        fclose(tmp_file);
        tmp_file = NULL;
        remove(DATA_FILE);
    }
    closelog();
}

// Signal handler for SIGINT and SIGTERM
void signal_handler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        stop_flag = 1;
        syslog(LOG_INFO, "Caught signal, exiting");
    }
}

// Function to create a daemon process
bool create_daemon() {
    pid_t pid = fork();
    if (pid < 0) {
        syslog(LOG_ERR, "Fork failed");
        return false;
    }
    if (pid > 0) {
        // Parent process exits
        exit(0);
    }

    // Create a new session
    if (setsid() == -1) {
        syslog(LOG_ERR, "Failed to create a new session");
        return false;
    }

    // Change working directory to root
    if (chdir("/") == -1) {
        syslog(LOG_ERR, "Failed to change directory");
        return false;
    }

    // Redirect stdin, stdout, and stderr to /dev/null
    int dev_null = open("/dev/null", O_RDWR);
    if (dev_null == -1) {
        syslog(LOG_ERR, "Failed to open /dev/null");
        return false;
    }
    dup2(dev_null, STDIN_FILENO);
    dup2(dev_null, STDOUT_FILENO);
    dup2(dev_null, STDERR_FILENO);
    close(dev_null);

    return true;
}

// Function to get a socket and bind it to the specified port
int get_listener_socket(const char *port) {
    struct addrinfo hints, *res;
    int sockfd;
    int yes = 1;

    // Set up hints structure
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;       // IPv4
    hints.ai_socktype = SOCK_STREAM; // TCP
    hints.ai_flags = AI_PASSIVE;     // Use my IP

    // Get address info
    if (getaddrinfo(NULL, port, &hints, &res) != 0) {
        syslog(LOG_ERR, "getaddrinfo failed");
        return -1;
    }

    // Create a socket
    if ((sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) == -1) {
        syslog(LOG_ERR, "Failed to create socket");
        freeaddrinfo(res);
        return -1;
    }

    // Allow reuse of the port
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes) == -1) {
        syslog(LOG_ERR, "Failed to set socket options");
        close(sockfd);
        freeaddrinfo(res);
        return -1;
    }

    // Bind the socket to the port
    if (bind(sockfd, res->ai_addr, res->ai_addrlen) == -1) {
        syslog(LOG_ERR, "Failed to bind socket: %s", strerror(errno));
        close(sockfd);
        freeaddrinfo(res);
        return -1;
    }

    freeaddrinfo(res);
    return sockfd;
}

int main(int argc, char *argv[]) {
    bool is_daemon = false;
    if (argc == 2 && strcmp(argv[1], "-d") == 0) {
        is_daemon = true;
    }

    // Open syslog
    openlog("aesdsocket", LOG_PID | LOG_CONS, LOG_USER);

    // Set up signal handlers
    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    // Get a listening socket
    if ((sockfd = get_listener_socket(PORT)) == -1) {
        cleanup();
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(sockfd, BACKLOG) == -1) {
        syslog(LOG_ERR, "Failed to listen on socket");
        cleanup();
        exit(EXIT_FAILURE);
    }

    // Daemonize if requested
    if (is_daemon && !create_daemon()) {
        cleanup();
        exit(EXIT_FAILURE);
    }

    // Open the data file
    tmp_file = fopen(DATA_FILE, "a+");
    if (tmp_file == NULL) {
        syslog(LOG_ERR, "Failed to open data file");
        cleanup();
        exit(EXIT_FAILURE);
    }

    // Main server loop
    while (!stop_flag) {
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof client_addr;
        int new_fd;

        // Accept a new connection
        if ((new_fd = accept(sockfd, (struct sockaddr *)&client_addr, &addr_len)) == -1) {
            syslog(LOG_ERR, "Failed to accept connection: %s", strerror(errno));
            continue;
        }

        // Log the client's IP address and port
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof client_ip);
        int client_port = ntohs(client_addr.sin_port);
        syslog(LOG_INFO, "Accepted connection from %s:%d", client_ip, client_port);

        // Receive data from the client
        char buf[BUF_SIZE];
        ssize_t bytes_received;
        while ((bytes_received = recv(new_fd, buf, BUF_SIZE - 1, 0)) > 0) {
            buf[bytes_received] = '\0';
            fputs(buf, tmp_file);
            fflush(tmp_file);

            // If a newline is found, send the file content back to the client
            if (strchr(buf, '\n') != NULL) {
                fseek(tmp_file, 0, SEEK_SET);
                char send_buf[BUF_SIZE];
                while (fgets(send_buf, BUF_SIZE, tmp_file) != NULL) {
                    send(new_fd, send_buf, strlen(send_buf), 0);
                }
            }
        }

        // Log connection closure
        syslog(LOG_INFO, "Closed connection from %s:%d", client_ip, client_port);

        // Close the connection
        close(new_fd);
    }

    // Cleanup and exit
    cleanup();
    return 0;
}
