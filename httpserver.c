// Asgn 4: A simple HTTP server.
// By:  Shaun Lagon

#include "listener_socket.h"
#include "httpserver.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// void handle_unsupported(conn_t *);
int parse_arguments(int argc, char **argv);

int main(int argc, char **argv) {
    int threads = parse_arguments(argc, argv);
    if (threads == -1) {
        return EXIT_FAILURE;
    }

    (void) threads;

    char *endptr = NULL;
    size_t port = (size_t) strtoull(argv[optind], &endptr, 10);
    if (endptr == argv[optind] && *endptr != '\0') {
        fprintf(stderr, "Invalid port number: %s\n", argv[1]);
        return EXIT_FAILURE;
    }

    signal(SIGPIPE, SIG_IGN);
    Listener_Socket_t *sock = ls_new(port);
    if (!sock) {
        fprintf(stderr, "cannot open socket\n");
        return EXIT_FAILURE;
    }

    while (1) {
        int connfd = ls_accept(sock);
        if (connfd < 0) {
            continue;
        }
        handle_connection(connfd);
        close(connfd);
    }
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
