#ifndef __HTTP_CONCURRENCY_H__
#define __HTTP_CONCURRENCY_H__
#include "myhttpd.h"

#define POOL_THREAD_SIZE 4

enum HTTP_CONCURRENCY_TYPE {
    ITERATIVE,
    FORK,
    THREAD,
    POOL_OF_THREADS
};
typedef void (*http_concurrency_handler)(int, http_handler);

void http_concurrency_iterative(int, http_handler);
void http_concurrent_fork(int, http_handler);
void init_http_concurrent_fork();
void http_concurrent_thread(int, http_handler);

#endif // !__HTTP_CONCURRENCY_H__