#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <pthread.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include "http-concurrency.h"
#include "myhttpd.h"

void http_concurrency_iterative(int socket, http_handler handler_func_ptr) {
    handler_func_ptr(socket);
}

// SIGCHLD handler
struct sigaction sigchld_action;
void sigchld_handler(int signum, siginfo_t * signal_info, void * context) {
  pid_t child_pid = signal_info->si_pid;
  int status;
  waitpid(child_pid, &status, WUNTRACED | WCONTINUED);
}

// init function for clearing up child process
void init_http_concurrent_fork() {
    // Configure sigaction for SIGCHLD
    sigchld_action.sa_sigaction = sigchld_handler;
    sigemptyset(&(sigchld_action.sa_mask));
    sigchld_action.sa_flags |= SA_RESTART | SA_SIGINFO;

    // Register signal handler
    if (sigaction(SIGCHLD, &sigchld_action, NULL)) {
        perror("Sigaction error");
        _Exit(1);
    }
}

// Create new process for each new request
void http_concurrent_fork(int socket, http_handler handler_func_ptr) {
    pid_t ret = fork();
    if (ret < 0) {
        perror("fork");
        _Exit(1);
    } else if (ret == 0) {
        // Child process, handle incoming request
        handler_func_ptr(socket);
        exit(0);
    }
    close(socket);
    fprintf(stderr, "[+] Created process %d to handle incoming request\n", ret);
    return;
}

void http_concurrent_thread(int socket, http_handler handler_func_ptr) {
    pthread_t t1;
	pthread_attr_t attr;
    pthread_attr_init( &attr );
	pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);
	int err = pthread_create(&t1, &attr, (void * (*)(void *)) handler_func_ptr, (void *) socket );
    if (err != 0) {
        perror("pthread");
    }
}