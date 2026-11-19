#include "uri_lock.h"

#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#define HASH_SIZE    128
#define RWLOCK_N_WAY 4

struct uri_node {
    char *uri;
    rwlock_t *rw;
    struct uri_node *next; // avoid hash collisons, singly linked list
};

static uri_node_t *buckets[HASH_SIZE];
static pthread_mutex_t table_mutex = PTHREAD_MUTEX_INITIALIZER;

// simple djb2 hash function, Stack Overflow
// https://stackoverflow.com/questions/743939/a-minimal-hash-function-for-c
static unsigned long hash_uri(const char *str) {
    unsigned long hash = 5381;
    int c = 0;
    while ((c = (unsigned char) *str++) != 0) {
        hash = ((hash << 5) + hash) + (unsigned long) c;
    }
    return hash % HASH_SIZE;
}

void uri_lock_init(void) {
    pthread_mutex_lock(&table_mutex);
    for (size_t i = 0; i < HASH_SIZE; i++) {
        buckets[i] = NULL;
    }
    pthread_mutex_unlock(&table_mutex);
}

rwlock_t *uri_lock_get(const char *uri) {
    if (uri == NULL) {
        return NULL;
    }

    unsigned long idx = hash_uri(uri);

    pthread_mutex_lock(&table_mutex);

    // Search existing bucket linked list items
    uri_node_t *curr = buckets[idx];
    while (curr != NULL) {
        if (strcmp(curr->uri, uri) == 0) {
            // Got table lock
            rwlock_t *rw = curr->rw;
            pthread_mutex_unlock(&table_mutex);
            return rw;
        }
        curr = curr->next;
    }

    // Not found, allocate a new entry
    uri_node_t *node = malloc(sizeof(uri_node_t));
    if (node == NULL) {
        pthread_mutex_unlock(&table_mutex);
        return NULL;
    }

    node->uri = strdup(uri);
    if (node->uri == NULL) {
        free(node);
        pthread_mutex_unlock(&table_mutex);
        return NULL;
    }

    node->rw = rwlock_new(N_WAY, RWLOCK_N_WAY);
    if (node->rw == NULL) {
        free(node->uri);
        free(node);
        pthread_mutex_unlock(&table_mutex);
        return NULL;
    }

    node->next = buckets[idx];
    buckets[idx] = node;

    rwlock_t *rw = node->rw;
    pthread_mutex_unlock(&table_mutex);
    return rw;
}

void uri_lock_destroy(void) {
    pthread_mutex_lock(&table_mutex);
    for (size_t i = 0; i < HASH_SIZE; i++) {
        uri_node_t *curr = buckets[i];
        while (curr != NULL) {
            uri_node_t *next = curr->next;
            rwlock_delete(&curr->rw);
            free(curr->uri);
            free(curr);
            curr = next;
        }
        buckets[i] = NULL;
    }
    pthread_mutex_unlock(&table_mutex);
    // immediately destroy table mutex
    pthread_mutex_destroy(&table_mutex);
}
