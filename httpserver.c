// Asgn 4: A simple HTTP server.
// By:  Shaun Lagon

#include "connection.h"
#include "listener_socket.h"
#include "request.h"
#include "response.h"

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/stat.h>

#define DEFAULT_THREADS 4

void handle_connection(int);

void handle_get(conn_t *);
void handle_put(conn_t *);
void handle_unsupported(conn_t *);
int parse_arguments(int argc, char **argv);

int main(int argc, char **argv) {
    int threads = parse_arguments(argc, argv);
    int opt = 0;

    
    if (threads == -1) {
      return EXIT_FAILURE;
    }

    (void) threads;

    char *endptr = NULL;
    size_t port = (size_t) strtoull(argv[1], &endptr, 10);
    if (endptr && *endptr != '\0') {
        warnx("invalid port number: %s", argv[1]);
        return EXIT_FAILURE;
    }

    signal(SIGPIPE, SIG_IGN);
    Listener_Socket_t *sock = ls_new(port);
    if (!sock) {
        warnx("cannot open socket");
        return EXIT_FAILURE;
    }

    while (1) {
        int connfd = ls_accept(sock);
        if (connfd < 0) continue;
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
int parse_arguments (int argc, char **argv) {
    while ((opt = getopt(argc, argv, "t:")) != -1) {
        switch (opt) {
            case 't':
                char *endptr = NULL;
                long val = strtol(optar, &endptr, 10);

                if (endptr === optarg || *endptr != '\0' || val <= 0) {
                    fprintf(stderr, "Invalid thread count\n");
                    return -1;
                }

                return (int) val;
            default:
                fprintf(stderr, "usage: %s [-t threads] <port>\n", argv[0]);
                return -1;
        }
    }

    if (optind != argc - 1) {
      fprintf(stderr, "usage: %s [-t threads] <port>\n", argv[0]);
      return -1;
    }
}

/*
 * @brief prints a line to stderr of the of an executed request
 * 
 * @param op HTTP operation that was performed
 * @param uri The uri to a file
 * @param status_code
 * @param req_id The optional HTTP header
*/
static void audit_log(
    const char *op,
    const char *uri,
    uint16_t status_code,
    const char *req_id) {
    
    const char *id = (req_id != NULL) ? req_id : "0";

    if (uri[0] == '/') {
      fprintf(stderr, "%s,%s,%u,$s\n", op, uri, status_code, id);
    } else {
      fprintf(stderr, "%s,/%s,%u,%s\n", op, uri, status_code, id);
    }
}

void handle_connection(int connfd) {

    conn_t *conn = conn_new(connfd);

    const Response_t *res = conn_parse(conn);

    if (res != NULL) {
        conn_send_response(conn, res);
    } else {
        printf("%s", conn_str(conn));
        const Request_t *req = conn_get_request(conn);
        if (req == &REQUEST_PUT) {
            handle_put(conn);
        }
        // add cases for other types of requests here
    }

    conn_delete(&conn);
}

void handle_put(conn_t *conn) {

    char *uri = conn_get_uri(conn);

    const Response_t *res = NULL;

    // Check if file already exists before opening it.
    bool existed = access(uri, F_OK) == 0;

    // Open the file
    int fd = open(uri, O_CREAT | O_WRONLY | O_TRUNC, 0600);

    if (fd < 0) {
        // printf("%s: %d", uri, errno);
        if (errno == EACCES || errno == EISDIR || errno == ENOENT) {
            res = &RESPONSE_FORBIDDEN;
            goto out;
        } else {
            res = &RESPONSE_INTERNAL_SERVER_ERROR;
            goto out;
        }
    }

    /* fd is valid */
    // receive the file
    res = conn_recv_file(conn, fd); // conn --> fd, NULL return is good

    // check to see if it was OK.
    if (res == NULL) {
        if (existed) {
            res = &RESPONSE_OK;
        } else {
            res = &RESPONSE_CREATED;
        }
    }

    close(fd);

out:
    conn_send_response(conn, res);
}
