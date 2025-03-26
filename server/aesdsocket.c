/***********************************************************************
* @file  aesdsocket.c
* @version 3
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
*	1 A6 P1 Changes for handling multiple simultaneous connections
*	2 A8 AESD char device support and previous assignment corrections
*	3 A9 Advanced Char Driver Operations
*
*Ref:
* 1. Lecture Videos
* 2. https://beej.us/guide/bgnet/html/
* 3. Chatgpt: Prompt: Socket creation and listening
* 4. A8 instructions
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
#include <pthread.h>
#include <time.h>  // Needed for time functions
#include "queue.h"
#include "../aesd-char-driver/aesd_ioctl.h"
#include <linux/ioctl.h>
#include "aesd_ioctl.h"

#define PORT "9000"    // Port to listen on
#define BACKLOG 10     // Max pending connections
#define BUF_SIZE 1024  // Buffer size for receiving data
#define TIMESTAMP_INTERVAL 10  // Interval for timestamp updates

/* Build switch for AESD char device */
#define USE_AESD_CHAR_DEVICE 1

#if (USE_AESD_CHAR_DEVICE == 1)
    #define DATA_FILE "/dev/aesdchar"
#else
    #define DATA_FILE "/var/tmp/aesdsocketdata"
#endif

volatile sig_atomic_t stop_flag = 0;
int sockfd = -1;  // Listening socket file descriptor

// Mutex to protect writes to DATA_FILE
pthread_mutex_t file_mutex = PTHREAD_MUTEX_INITIALIZER;

// Define the thread node using SLIST macros from queue.h
typedef struct thread_node {
    pthread_t thread;                  // Thread handle
    int client_fd;                     // Client socket file descriptor
    SLIST_ENTRY(thread_node) entries;  // Linked list entry
} thread_node_t;

// Define and initialize the head of our singly linked list
SLIST_HEAD(thread_list_head_s, thread_node);
struct thread_list_head_s thread_list_head = SLIST_HEAD_INITIALIZER(thread_list_head);

// Timer thread ID
pthread_t timer_thread_id;

// Function to handle cleanup on exit
void cleanup() {
    if (sockfd != -1) {
        shutdown(sockfd, SHUT_RDWR);  // Gracefully shutdown socket before closing
        close(sockfd);
        sockfd = -1;
    }
    #if (USE_AESD_CHAR_DEVICE == 0)
        remove(DATA_FILE);
    #endif
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

// Thread function that handles an individual client connection
void *connection_handler(void *arg) {
    thread_node_t *node = (thread_node_t *)arg;
    int client_fd = node->client_fd;
    char buf[BUF_SIZE];
	char message_buffer[BUF_SIZE];  // Buffer to store complete messages
    ssize_t bytes_received;
	size_t message_length = 0;

	while ((bytes_received = recv(client_fd, buf, BUF_SIZE, 0)) > 0) {
	    pthread_mutex_lock(&file_mutex);  // Lock before file operations

	    for (ssize_t i = 0; i < bytes_received; i++) {
	   	    // Prevent buffer overflow
		    if (message_length < BUF_SIZE - 1) {
	            message_buffer[message_length++] = buf[i];
            }
            // Message complete, write to file
	        if (buf[i] == '\n') {  // Message complete, write to file
	            message_buffer[message_length] = '\0';

#if USE_AESD_CHAR_DEVICE
		    // Check for IOCTL command before writing
		    if (strncmp(message_buffer, "AESDCHAR_IOCSEEKTO:", 20) == 0) {
		        int x, y;
		        if (sscanf(message_buffer, "AESDCHAR_IOCSEEKTO:%d,%d", &x, &y) == 2) {
		            struct aesd_seekto seekto = {
		                .write_cmd = x,
		                .write_cmd_offset = y
		            };
		
		            int fd = open(DATA_FILE, O_RDWR);
		            if (fd < 0) {
		                syslog(LOG_ERR, "Failed to open aesdchar for ioctl: %s", strerror(errno));
		            } else {
		                if (ioctl(fd, AESDCHAR_IOCSEEKTO, &seekto) == -1) {
		                    syslog(LOG_ERR, "IOCTL AESDCHAR_IOCSEEKTO failed: %s", strerror(errno));
		                }
		                close(fd);
		            }
		        }
		    } else
#endif
    		{
    			// Normal write to device/file
	            FILE *fp = fopen(DATA_FILE, "a+");
	            if (fp) {
	                fputs(message_buffer, fp);  // Write only complete messages
	                fclose(fp);
	            } else {
	                syslog(LOG_ERR, "Failed to open data file: %s", strerror(errno));
	            }
	        }    

            // Shift remaining data to the start of the buffer (if any)
            size_t remaining_length = bytes_received - i - 1;
            memmove(message_buffer, &buf[i + 1], remaining_length);
            message_length = remaining_length;  // Adjust length
            }
        }

        pthread_mutex_unlock(&file_mutex);  // Unlock after file operations
    }

    // Send the contents of the file back to the client
    pthread_mutex_lock(&file_mutex);  // Lock before reading from file
    FILE *fp = fopen(DATA_FILE, "r");
    if (fp) {
        char send_buf[BUF_SIZE];
        size_t n;
        while ((n = fread(send_buf, 1, BUF_SIZE, fp)) > 0) {
            if (send(client_fd, send_buf, n, 0) == -1) {
                syslog(LOG_ERR, "Failed to send data to client: %s", strerror(errno));
                break;
            }
        }
        fclose(fp);
    } else {
        syslog(LOG_ERR, "Failed to open data file for reading: %s", strerror(errno));
    }
    pthread_mutex_unlock(&file_mutex);  // Unlock after reading

    if (bytes_received == 0) {
        syslog(LOG_INFO, "Client disconnected");
    } else if (bytes_received < 0) {
        syslog(LOG_ERR, "recv() failed: %s", strerror(errno));
    }

    close(client_fd);
    return NULL;

}

// Timer thread function that appends a timestamp every 10 seconds.
void *timer_thread(void *arg) {
    (void)arg;
    struct timespec next_wakeup;
    clock_gettime(CLOCK_MONOTONIC, &next_wakeup);

    while (!stop_flag) {
        next_wakeup.tv_sec += TIMESTAMP_INTERVAL;
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next_wakeup, NULL);

        #if (USE_AESD_CHAR_DEVICE == 0)  
        time_t now = time(NULL);
        struct tm *tm_info = localtime(&now);
        char time_string[128];
        strftime(time_string, sizeof(time_string), "timestamp:%a, %d %b %Y %H:%M:%S %z\n", tm_info);

        pthread_mutex_lock(&file_mutex);
        FILE *fp = fopen(DATA_FILE, "a+");
        if (fp) {
            syslog(LOG_DEBUG, "Writing timestamp to file: %s", time_string);
            fputs(time_string, fp);
            fflush(fp);
            fclose(fp);
        } else {
            syslog(LOG_ERR, "Failed to open data file for timestamp: %s", strerror(errno));
        }
        pthread_mutex_unlock(&file_mutex);
        #endif
    }
    return NULL;
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
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    #if (USE_AESD_CHAR_DEVICE == 0)
        FILE *fp = fopen(DATA_FILE, "w");
        if (fp) fclose(fp);
    #endif

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

    // Create the timer thread in the parent process.
    if (pthread_create(&timer_thread_id, NULL, timer_thread, NULL) != 0) {
        syslog(LOG_ERR, "Failed to create timer thread: %s", strerror(errno));
        cleanup();
        exit(EXIT_FAILURE);
    }

    // Main server loop: accept connections until stop_flag is set
    while (!stop_flag) {
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);
        int new_fd = accept(sockfd, (struct sockaddr *)&client_addr, &addr_len);

        if (new_fd == -1) {
            if (errno == EINTR) continue;
            syslog(LOG_ERR, "Failed to accept connection: %s", strerror(errno));
            continue;
        }

        // Allocate a new thread node and insert it into our SLIST.
        thread_node_t *node = malloc(sizeof(thread_node_t));
        if (!node) {
            syslog(LOG_ERR, "Failed to allocate memory for thread node");
            close(new_fd);
            continue;
        }
        node->client_fd = new_fd;
        SLIST_INSERT_HEAD(&thread_list_head, node, entries);

        if (pthread_create(&node->thread, NULL, connection_handler, node) != 0) {
            syslog(LOG_ERR, "Failed to create client thread: %s", strerror(errno));
            close(new_fd);
            free(node);
            continue;
        }

        pthread_detach(node->thread); // Ensure thread cleans up after execution
    }

    // Stop accepting new connections.
    close(sockfd);

    // Join timer thread before cleanup
    if (pthread_join(timer_thread_id, NULL) != 0) {
        syslog(LOG_ERR, "Failed to join timer thread: %s", strerror(errno));
    }

    // Join all client threads before exit
    thread_node_t *node, *temp_node;
    SLIST_FOREACH_SAFE(node, &thread_list_head, entries, temp_node) {
        pthread_join(node->thread, NULL);  // Ensure all threads complete
        SLIST_REMOVE(&thread_list_head, node, thread_node, entries);
        free(node);
    }
    // Cleanup and exit
    cleanup();
    return 0;
}

