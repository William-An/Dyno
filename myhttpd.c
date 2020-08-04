#include <sys/types.h>
#include <sys/socket.h>
#if __linux___
	#include <sys/sendfile.h>
#endif
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <pthread.h>
#include "myhttpd.h"
#include "auth.h"
#include "http-concurrency.h"

const char * usage =
"                                                               \n"
"http-server:                                                   \n"
"                                                               \n"
"Simple http server program                                     \n"
"                                                               \n"
"To use it in one window type:                                  \n"
"                                                               \n"
"   myhttpd [-f|-t|-p] [<port>]                                 \n";

int QueueLength = 5;

server_config myserver_t;
server_config * myserver;

const char * pwfile = "./mycredentials.txt";

// MIME Mapping table
// TODO Better approach
#define MIME_LENGTH 9
const char * const file_extension[] = {".html", ".bmp", ".gif", ".ico", ".png", ".jpg", ".xbm", ".c", ".o"};
const char * const MIME[] = {"text/html", "image/bmp", "image/gif", "image/vnd.microsoft.icon", "image/png", "image/jpeg", "image/x-xbitmap", "text/plain", "application/octet-stream"};

// CS 252 Lab only
pthread_mutex_t mutex;

int main( int argc, char **argv) {
    // Parsing arguments
    enum HTTP_CONCURRENCY_TYPE http_concurrency_t = ITERATIVE;
    char opt;
    fprintf(stderr, "[-] Parsing arguments...\n");
    while ((opt = getopt(argc, argv, "ftp")) != -1) {
        switch (opt) {
        case 'f':
            http_concurrency_t = FORK;
            break;
        case 't':
            http_concurrency_t = THREAD;
            break;
        case 'p':
            http_concurrency_t = POOL_OF_THREADS;
            break;
        default: /* '?' */
            fprintf(stderr, "%s", usage);
            exit(EXIT_FAILURE);
        }
    }
    fprintf(stderr, "[+] Argument parsing completed\n");

    http_concurrency_handler req_handler_func;
    switch (http_concurrency_t) {            
        case FORK:
            init_http_concurrent_fork();
            req_handler_func = &http_concurrent_fork;
            break;
        case THREAD:
            req_handler_func = &http_concurrent_thread;
            break;
        case POOL_OF_THREADS:
            break;
        default:
            req_handler_func = &http_concurrency_iterative;
    } 

    // Get the port from the arguments
    char * tmp;
    errno = 0;
    int port = strtol(argv[argc - 1], &tmp, 10);
    if (errno != 0) {
        fprintf(stderr, "Invalid Port number, must be number between 1024 and 65536\n");
        fprintf(stderr, "%s", usage);
        exit(EXIT_FAILURE);
    }
    // Check for port number
    if (tmp != argv[argc - 1]) {
        if (port <= 1024 || port >= 65536) {
            fprintf(stderr, "Invalid Port number, must be number between 1024 and 65536\n");
            exit(EXIT_FAILURE);
        }
    } else {
        // No port num found, randomly assigned one   
        port = random() % (65536 - 1025) + 1025;
    }

    // Init server struct
    myserver = &myserver_t;
    myserver->root_dir = "./http-root-dir";
    myserver->index_path = "/htdocs/index.html";

    // Set the IP address and port for this server
    struct sockaddr_in serverIPAddress; 
    memset( &serverIPAddress, 0, sizeof(serverIPAddress) );
    serverIPAddress.sin_family = AF_INET;
    serverIPAddress.sin_addr.s_addr = INADDR_ANY;
    serverIPAddress.sin_port = htons((u_short) port);

    // Allocate a socket
    int masterSocket =  socket(PF_INET, SOCK_STREAM, 0);
    if ( masterSocket < 0) {
        perror("socket");
        exit( -1 );
    }

    // Set socket options to reuse port. Otherwise we will
    // have to wait about 2 minutes before reusing the sae port number
    int optval = 1; 
    int err = setsockopt(masterSocket, SOL_SOCKET, SO_REUSEADDR, 
                (char *) &optval, sizeof( int ) );

    // Bind the socket to the IP address and port
    int error = bind( masterSocket,
            (struct sockaddr *)&serverIPAddress,
            sizeof(serverIPAddress) );
    if ( error ) {
        perror("bind");
        exit( -1 );
    }

    fprintf(stderr, "[+] Assigned port is %d\n", port);

    // Put socket in listening mode and set the 
    // size of the queue of unprocessed connections
    error = listen( masterSocket, QueueLength);
    if ( error ) {
        perror("listen");
        exit( -1 );
    }

    fprintf(stderr, "[+] Listening on %d\n", port);

    if (http_concurrency_t != POOL_OF_THREADS) {
        while ( 1 ) {
            // Accept incoming connections
            struct sockaddr_in clientIPAddress;
            int alen = sizeof( clientIPAddress );
            int slaveSocket = accept( masterSocket,
                            (struct sockaddr *)&clientIPAddress,
                            (socklen_t*)&alen);

            if ( slaveSocket < 0 ) {
                perror( "accept" );
                exit( -1 );
            }

            // Print incoming socket info
            fprintf(stderr, "[+] Get connenction from: %s\n", inet_ntoa(clientIPAddress.sin_addr));

            // TODO Add middleware functions
            // Process request.
            req_handler_func(slaveSocket, processRequest);
        }
    } else {
	    pthread_mutex_init(&mutex, NULL);
        pthread_t threads[4];
        for (int i = 0; i < 4; i++) {
            pthread_create(&threads[i], NULL, (void * (*)(void *)) http_concurrent_thread_pool_loop, (void *)masterSocket);
        }
        http_concurrent_thread_pool_loop(masterSocket);
    }
}

// Fd get line separated by delimiter
// TODO Why wont ../ work?
char * getline_del(int fd, const char * delimiter) {
    size_t del_len = strlen(delimiter);
    int buf_size = 10;
    int tail = 0;
    char * buf = (char *)malloc(sizeof(char) * buf_size);
    while(read(fd, &buf[tail], 1) > 0) {
        tail++;
        if (tail == buf_size) {
            // Realloc space for buffer
            buf_size *= 2;
            buf = (char *)realloc(buf, sizeof(char) * buf_size);
        }
        // Compare the end of string with delimiter
        buf[tail] = '\0';   // End string for comparison
        if (tail >= del_len) {   // start string comparison if more than del_len
            char * substr = &buf[tail - del_len];
            if (strcmp(substr, delimiter) == 0) {
                // Remove the delimiter
                buf[tail - del_len] = '\0';
                break;
            }
        }
    }
    return buf;
}

// Put header info in struct
void processRequestHeader(int socket, http_request_s * req, http_response_s * res) {
    // Parsing first line
    char * protocol_line = getline_del(socket, "\r\n");
    char * saveptr;
    char * req_type = strtok_r(protocol_line, " ", &saveptr);
    enum HTTP_REQUEST_TYPE http_req_type;
    int isValid = 1;
    if (req_type == NULL) {
        res->status_code = BAD_REQUEST;
        res->status_msg = strdup("BAD REQUEST");
        res->error_msg = strdup("Invalid request type\n");
        isValid = 0;
    }
    else if (!strcmp(req_type, "GET"))
        http_req_type = GET;
    else if (!strcmp(req_type, "POST"))
        http_req_type = POST;
    else {
        // TODO Send error msg response immediately
        // and return
        res->status_code = BAD_REQUEST;
        res->status_msg = strdup("BAD REQUEST");
        res->error_msg = strdup("Invalid request type\n");
        isValid = 0;
    }

    if (isValid) {
        req->req_t = http_req_type;
        req->req_dir = strdup(strtok_r(NULL, " ", &saveptr));
        req->req_protocol = strdup(strtok_r(NULL, " ", &saveptr));
    } else {
        req->req_t = INVALID;
        req->req_dir = NULL;
        req->req_protocol = NULL;
    }
    free(protocol_line);

    #ifdef DEBUG
    fprintf(stderr, "[+] Request type: %d\n", req->req_t);
    fprintf(stderr, "[+] Request dir: %s\n", req->req_dir);
    fprintf(stderr, "[+] Request protocol: %s\n", req->req_protocol);
    #endif
    
    // Parsing header
    char * header_line;
    while(isValid) {
        header_line = getline_del(socket, "\r\n");
        if (*header_line == '\0')   // reach end of header
            break;
        else {
            req->req_header[req->req_header_length] = header_line;
            req->req_header_length++;
            if (req->req_header_length == req->max_req_header_length) {
                req->max_req_header_length *= 2;
                req->req_header = (char **)realloc(req->req_header, sizeof(char *) * req->max_req_header_length);
            }
        }
    }

    // Debug output
    #ifdef DEBUG
    fprintf(stderr, "Request header info\n");
    for (size_t i = 0; i < req->req_header_length; i++)
        fprintf(stderr, "[+] Request header info: %s\n", req->req_header[i]);
    #endif
}

// Put header info in response header
void processResponse(int socket, http_request_s * req, http_response_s * res) {
    res->socket = socket;

    // Currently just serve file under root dir
    // In future add more support
    if (res->status_code != DEFAULT) {
        res->data_fd = -1;
    } else {
        // Auth checking
        int isVerified = basic_auth(pwfile, req, res);
        if (!isVerified) {
            // Authorization header not found or wrong credential
            res->status_code = UNAUTHORIZED;
            res->status_msg = strdup("Unauthorizerd");
            res->res_header[res->res_header_length] = strdup("WWW-Authenticate: Basic realm=\"myhttpd-cs252\"");
            res->res_header_length++;
            res->data_fd = -1;
        } else if (!strcmp(req->req_dir, "/") || !strcmp(req->req_dir, "/index.html")) {
            // Redirect incoming request
            res->status_code = MOVED_PERM;
            res->status_msg = strdup("Moved Permanently");
            char buf[1024];
            sprintf(buf, "Location: %s", myserver->index_path);
            res->res_header[res->res_header_length] = strdup(buf);
            res->res_header_length++;
            res->data_fd = -1;
        } else {
            // Search root dir
            // Open fd
            char * buf = (char *)malloc(sizeof(char) * (strlen(myserver->root_dir) + strlen(req->req_dir) + 1));
            buf[0] = '\0';
            strcat(buf, myserver->root_dir);
            // TODO Check for ".."
            // TODO Handle subdir access?
            strcat(buf, req->req_dir);
            int fd = open((const char *)buf, O_RDONLY);
            free(buf);
            // Set header info accordingly
            res->data_fd = fd;
            if (fd == -1) {
                // Open error
                res->status_code = NOT_FOUND;
                res->status_msg = strdup("FILE NOT FOUND");
                char tmp[1024];
                sprintf(tmp, "%s not found\n", req->req_dir);
                res->error_msg = strdup(tmp);
                res->res_header[res->res_header_length] = strdup("Content-type: text/plain");
                res->res_header_length++;
            } else {
                // Set header info
                res->status_code = OK;
                res->status_msg = strdup("OK");

                // MIME Parsing
                char * ext = strrchr(req->req_dir, '.');
                char tmp[1024];
                if (ext == NULL) {
                    sprintf(tmp, "Content-type: %s", "application/octet-stream");
                } else {
                    for (int index = 0; index < MIME_LENGTH; index++) {
                        if (!strcmp(file_extension[index], ext)) {
                            sprintf(tmp, "Content-type: %s", MIME[index]);
                            break;
                        }
                    }
                }
                res->res_header[res->res_header_length] = strdup(tmp);
                res->res_header_length++;
            }
            res->res_header[res->res_header_length] = strdup("Server: CS 252 Lab 5");
            res->res_header_length++;
        }
    }

    #ifdef DEBUG
    fprintf(stderr, "Response info\n");
    fprintf(stderr, "Response status info\n");
    fprintf(stderr, "[+] Status code: %d\n", res->status_code);
    fprintf(stderr, "Response Header info\n");
    for (size_t i = 0; i < res->res_header_length; i++)
        fprintf(stderr, "[+] Response header info: %s\n", res->res_header[i]);
    fprintf(stderr, "------------------------\n");
    #endif
}

void sendResponse(int socket, http_request_s * req, http_response_s * res) {
    // Send header
    char * crlf = "\r\n";

    // First line
    char buf[1024];
    if (req->req_protocol == NULL)
        req->req_protocol = strdup("HTTP/1.1");
    sprintf(buf, "%s %d %s ", req->req_protocol, res->status_code, res->status_code == OK ? req->req_dir : res->status_msg);
    send(socket, &buf, strlen(buf), 0);
    send(socket, crlf, 2, 0);

    // Send rest header info
    for (int i = 0; i < res->res_header_length; i++) {
        char * tmp = res->res_header[i];
        send(socket, tmp, strlen(tmp), 0);
        send(socket, crlf, 2, 0);
    }
    send(socket, crlf, 2, 0);
    
    // Send data or error_msg
    if (res->data_fd == -1 && res->error_msg != NULL) {
        // Send error msg
        send(socket, res->error_msg, strlen(res->error_msg), 0);
    } else {
        // Serve file
        off_t count = lseek(res->data_fd, 0, SEEK_END);
        lseek(res->data_fd, 0, SEEK_SET);
	#if __linux__
		sendfile(socket, res->data_fd, NULL, count);
	#elif __APPLE__
		int err = sendfile(res->data_fd, socket, 0, &count, NULL, 0);
		if (err)
            perror("Sendfile");
	#endif	
	close(res->data_fd);
    }
}

void processRequest(int socket) {
    http_request_s * req = init_http_request();
    http_response_s * res = init_http_response();
    processRequestHeader(socket, req, res);
    processResponse(socket, req, res);
    sendResponse(socket, req, res);
    delete_http_request(req);
    delete_http_response(res);
    close(socket);
}


// Initation and destroyer
http_request_s * init_http_request() {
    http_request_s * req = (http_request_s *)malloc(sizeof(http_request_s));
    memset(req, 0, sizeof(http_request_s));
    req->req_header = (char **) malloc(sizeof(char *) * INIT_HEADER_LENGTH);
    req->req_header_length = 0;
    req->max_req_header_length = INIT_HEADER_LENGTH;
    return req;
}

void delete_http_request(http_request_s * req) {
    for (size_t i = 0; i < req->req_header_length; i++)
        free(req->req_header[i]);   // freeing each header line
    free(req->req_header);
    free(req->req_dir);
    free(req->req_protocol);
    free(req);
}

http_response_s * init_http_response() {
    http_response_s * res = (http_response_s *)malloc(sizeof(http_response_s));
    memset(res, 0, sizeof(http_response_s));
    res->status_code = DEFAULT;
    res->error_msg = NULL;
    res->res_header = (char **) malloc(sizeof(char *) * INIT_HEADER_LENGTH);
    res->res_header_length = 0;
    res->max_res_header_length = INIT_HEADER_LENGTH;
    return res;
}

void delete_http_response(http_response_s * res) {
    for (size_t i = 0; i < res->res_header_length; i++)
        free(res->res_header[i]);   // freeing each header line
    free(res->res_header);
    free(res->status_msg);
    free(res->error_msg);
    free(res);
}

// For CS 252 Lab only
void http_concurrent_thread_pool_loop(int masterSocket) {
    while (1) {
        // todo add mutex
        struct sockaddr_in clientIPAddress;
        int alen = sizeof( clientIPAddress );
        pthread_mutex_lock(&mutex);
        int slaveSocket = accept(masterSocket, (struct sockaddr *)&clientIPAddress, (socklen_t*)&alen);
        pthread_mutex_unlock(&mutex);
        if (slaveSocket >= 0) { 
            processRequest(slaveSocket);
        } 
    }
}
