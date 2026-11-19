// Asgn 4: A simple HTTP server.
// By:  Shaun Lagon

#include "listener_socket.h"
#include "conn_handlers.h"
#include "thread_handlers.h"
#include "uri_lock.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>

int parse_arguments(int argc, char **argv);

int main(int argc, char **argv) {
    int threads = parse_arguments(argc, argv);
    if (threads == -1) {
        return EXIT_FAILURE;
    }

    char *endptr = NULL;
    size_t port = (size_t) strtoull(argv[optind], &endptr, 10);
    if (endptr == argv[optind] || *endptr != '\0') {
        fprintf(stderr, "Invalid port number: %s\n", argv[optind]);
        return EXIT_FAILURE;
    }

    signal(SIGPIPE, SIG_IGN);

    // Register SIGTERM and SIGINT without SA_RESTART so ls_accept unblocks
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sigterm_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGINT, &sa, NULL);

    Listener_Socket_t *sock = ls_new(port);
    if (!sock) {
        fprintf(stderr, "cannot open socket\n");
        return EXIT_FAILURE;
    }

    work_q = queue_new(threads);
    if (work_q == NULL) {
        ls_delete(&sock);
        return EXIT_FAILURE;
    }
    uri_lock_init();

    pthread_t *workers = malloc(sizeof(pthread_t) * (size_t) threads);
    if (workers == NULL) {
        uri_lock_destroy();
        queue_delete(&work_q);
        ls_delete(&sock);
        return EXIT_FAILURE;
    }

    if (init_worker_threads(threads, workers) < 0) {
        uri_lock_destroy();
        queue_delete(&work_q);
        ls_delete(&sock);
        free(workers);
        fprintf(stderr, "issue creating worker threads\n");
        return EXIT_FAILURE;
    }

    while (running) {
        int connfd = ls_accept(sock);
        if (connfd < 0) {
            continue;
        }
        queue_push(work_q, (void *) (intptr_t) connfd);
    }

    kill_threads(threads, workers, work_q);
    fflush(stderr);
    free(workers);
    uri_lock_destroy();
    queue_delete(&work_q);
    ls_delete(&sock);

    return EXIT_SUCCESS;
}

/*
 * @brief Helper function using getopt to parse command line arguments
 * 
 * @param argc number of arguments passed in
 * @param argv array of strings of command line arguments
 * 
 * @return int where non-negative integer <= 4 is success and indicates the
 *         number of threads to use, -1 indicates a failure
*/
int parse_arguments(int argc, char **argv) {
    int opt = 0;
    long val = DEFAULT_THREADS;
    while ((opt = getopt(argc, argv, "t:")) != -1) {
        switch (opt) {
        case 't': {
            char *endptr = NULL;
            val = strtol(optarg, &endptr, 10);

            if (endptr == optarg || *endptr != '\0' || val <= 0) {
                fprintf(stderr, "Invalid thread count\n");
                return -1;
            }

            break;
        }
        default:
            fprintf(stderr, "usage: %s [-t threads] <port>\n", argv[0]);
            return -1;
        }
    }

    if (optind != argc - 1) {
        fprintf(stderr, "usage: %s [-t threads] <port>\n", argv[0]);
        return -1;
    }

    return (int) val;
}
