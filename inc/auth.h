#ifndef __AUTH__H__
#define __AUTH__H__
#include "myhttpd.h"
#include <stdio.h>

int basic_auth(const char * pwfilename, http_request_s * req, http_response_s * res);

#endif // !__AUTH__H__
