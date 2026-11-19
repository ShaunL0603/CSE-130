#ifndef CONN_HANDLERS_H
#define CONN_HANDLERS_H

#include <stdint.h>

#define DEFAULT_THREADS 4

void handle_connection(int connfd);
void audit_log(
    const char *op, const char *uri, uint16_t status_code, const char *req_id);

#endif
