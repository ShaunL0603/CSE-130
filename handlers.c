// C file to handle connections
// By:  Shaun Lagon

#include "handlers.h"
#include "connection.h"
#include "request.h"
#include "response.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdint.h>

static void handle_get(conn_t *);
static void handle_put(conn_t *);

/*
 * @brief prints a line to stderr of the of an executed request
 *
 * @param op HTTP operation that was performed
 * @param uri The uri to a file
 * @param status_code
 * @param req_id The optional HTTP header
*/
void audit_log(
    const char *op, const char *uri, uint16_t status_code, const char *req_id) {

    const char *id = (req_id != NULL) ? req_id : "0";

    if (uri[0] == '/') {
        fprintf(stderr, "%s,%s,%u,%s\n", op, uri, status_code, id);
    } else {
        fprintf(stderr, "%s,/%s,%u,%s\n", op, uri, status_code, id);
    }
}

void handle_connection(int connfd) {
    conn_t *conn = conn_new(connfd);
    if (conn == NULL)
        return;

    const Response_t *res = conn_parse(conn);

    if (res != NULL) {
        conn_send_response(conn, res);
    } else {
        // printf("%s", conn_str(conn));
        const Request_t *req = conn_get_request(conn);
        if (req == &REQUEST_PUT) {
            handle_put(conn);
        } else if (req == &REQUEST_GET) {
            handle_get(conn);
        } else {
            conn_send_response(conn, &RESPONSE_NOT_IMPLEMENTED);
        }
    }

    conn_delete(&conn);
}

static void handle_put(conn_t *conn) {

    char *uri = conn_get_uri(conn);

    char *req_id = conn_get_header(conn, "Request-Id");

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
    audit_log("PUT", uri, response_get_code(res), req_id);
    goto out;
out:
    conn_send_response(conn, res);
}

/*
 * @brief handle the http GET operation using helper functions defined in
 *        the "connection.h" header file
 * 
 * @param connection socket API
*/
static void handle_get(conn_t *conn) {
    const char *uri = conn_get_uri(conn);
    char *req_id = conn_get_header(conn, "Request-Id");
    const Response_t *res = NULL;

    int fd = open(uri, O_RDONLY);
    if (fd < 0) {
        if (errno == ENOENT) {
            res = &RESPONSE_NOT_FOUND;
        } else if (errno == EACCES) {
            res = &RESPONSE_FORBIDDEN;
        } else {
            res = &RESPONSE_INTERNAL_SERVER_ERROR;
        }

        audit_log("GET", uri, response_get_code(res), req_id);
        conn_send_response(conn, res);
        return;
    }

    // struct to store and contain metadata about a file or directory
    // from <sys/stat.h> library
    struct stat st;
    if (fstat(fd, &st) < 0) {
        close(fd);
        res = &RESPONSE_INTERNAL_SERVER_ERROR;
        audit_log("GET", uri, response_get_code(res), req_id);
        conn_send_response(conn, res);
        return;
    }

    if (S_ISDIR(st.st_mode)) {
        close(fd);
        res = &RESPONSE_FORBIDDEN;
        audit_log("GET", uri, response_get_code(res), req_id);
        conn_send_response(conn, res);
        return;
    }

    // valid file
    audit_log("GET", uri, response_get_code(&RESPONSE_OK), req_id);
    conn_send_file(conn, fd, (uint64_t) st.st_size);
    close(fd);
}
