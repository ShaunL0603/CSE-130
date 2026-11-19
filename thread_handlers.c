#include "uri_lock.h"
#include "queue.h"
#include "rwlock.h"
#include "conn_handlers.h"

#include <string.h>
#include <stdint.h>
#include <pthread.h>
#include <unistd.h>

#define DEFAULT_THREADS 4

volatile __sig_atomic_t running = 1;
queue_t *work_q = NULL;

void kill_threads(int threads, pthread_t *workers, queue_t *q) {
    for (int i = 0; i < threads; ++i) {
        queue_push(q, (void *) (intptr_t) -1);
    }
    for (int i = 0; i < threads; ++i) {
        pthread_join(workers[i], NULL);
    }
}

void sigterm_handler(int sig) {
    (void) sig;
    running = 0;
}

void *worker_thread(void *arg) {
    (void) arg;

    while (1) {
        void *elem = NULL;
        if (!queue_pop(work_q, &elem)) {
            break;
        }
        int connfd = (int) (intptr_t) elem;
        if (connfd == -1) {
            break;
        }

        handle_connection(connfd);
        close(connfd);
    }
    return NULL;
}

int init_worker_threads(int threads, pthread_t *workers) {
    for (int i = 0; i < threads; ++i) {
        if (pthread_create(&workers[i], NULL, worker_thread, NULL) != 0) {
            // push -1 to let workers finish instead of stopping midway.
            // connfd expects values >= 0
            kill_threads(i, workers, work_q);
            return -1;
        }
    }
    return 0;
}
