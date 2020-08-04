#include "auth.h"
#include <string.h>

// Open file with `pwfilename` and check if credential is in the file
// 1 if is in, 0 other wise
int basic_auth_verify(const char * pwfilename, char * credential) {
    int res = 0;
    if (credential == NULL)
        return res;
    FILE * pwfp = fopen(pwfilename, "r");
    if (pwfp == NULL) {
        fprintf(stderr, "[!] Password file: %s not found\n", pwfilename);
        return res;
    }
    char buf[1024];
    while(!feof(pwfp)) {
        fgets(buf, 1023, pwfp);
        if (!strcmp(buf, credential)) {
            // Found matched
            res = 1;
        }
    }

    // Clean up
    fclose(pwfp);
    return res;
}

// Extract Authorization header from the request header
int basic_auth(const char * pwfilename, http_request_s * req, http_response_s * res) {
    int verify_res = 0;
    char * credential = NULL;
    for (size_t i = 0; i < req->req_header_length; i++) {
        char * header_line = req->req_header[i];
        if (!strncasecmp("Authorization:", header_line, strlen("Authorization:"))) {
            char * saveptr;   
            // Assume header_line in form of "Authorization: Basic BASE64"
            strtok_r(header_line, " ", &saveptr);
            strtok_r(NULL, " ", &saveptr);
            credential =  strtok_r(NULL, " ", &saveptr);
            break;
        }
    }
    if (credential != NULL)
        verify_res = basic_auth_verify(pwfilename, credential);
    return verify_res;
}