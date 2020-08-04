#ifndef __MYHTTPD_H__
#define __MYHTTPD_H__
#include <stdio.h>

#define INIT_HEADER_LENGTH 10

typedef void (*http_handler)(int);
typedef struct server_config_t {
    const char * root_dir;
    const char * index_path;
} server_config;

enum HTTP_REQUEST_TYPE {
    INVALID = -1,
    GET,
    POST
};

enum HTTP_STATUS_CODE {
    DEFAULT = -1,
    CONTINUE = 100,
    SWITCH_PROC = 101,
    OK = 200,
    CREATED = 201,
    ACCEPTED = 202,
    NON_AUTH_INFO = 203,
    NO_CONTENT = 204,
    RESET_CONTENT = 205,
    PARTIAL_CONTENT = 206,
    MULTIPLE_CHOICE = 300,
    MOVED_PERM = 301,
    FOUND = 302,
    BAD_REQUEST = 400,
    UNAUTHORIZED = 401,
    FORBIDDEN = 403,
    NOT_FOUND = 404,
    INTERN_SERVER_ERROR = 500
};

// TODO Make header in form of dictionary
typedef struct request_t {
    int socket;
    enum HTTP_REQUEST_TYPE req_t;
    char * req_dir;
    char * req_protocol;
    char ** req_header;
    size_t req_header_length;
    size_t max_req_header_length;
} http_request_s;

typedef struct response_t {
    int socket;
    enum HTTP_STATUS_CODE status_code;
    char * status_msg;
    char ** res_header;
    size_t res_header_length;
    size_t max_res_header_length;
    char * error_msg;
    int data_fd;    // File fd to be served
} http_response_s;

// Request and response struct init and destory
http_request_s * init_http_request();
void delete_http_request(http_request_s *);
http_response_s * init_http_response();
void delete_http_response(http_response_s *);

// Process request and response
void processRequest(int socket);
void processRequestHeader(int socket, http_request_s * req, http_response_s * res);
void processResponse(int socket, http_request_s * req, http_response_s * res);
void sendResponse(int socket, http_request_s * req, http_response_s * res);

// FOR CS 252 Lab only, could remove
void http_concurrent_thread_pool_loop(int);

// TODO Handler function for different http request types?

#endif // !__MYHTTPD_H__