#ifndef THREAD_HANDLERS_H
#define THREAD_HANDLERS_H

#include "queue.h"
#include "conn_handlers.h"

#include <pthread.h>
#include <signal.h>
#include <unistd.h>

extern volatile __sig_atomic_t running;
extern queue_t *work_q;

/**
 * @brief helper function to start terminating process of queue and threads
 *
 * @params threads number of threads specified
 * @params workers unique thread identifier
 * @params q the initalized queue
 */
void kill_threads(int threads, pthread_t *workers, queue_t *q);

void sigterm_handler(int sig);
void *worker_thread(void *arg);

/**
 * @brief helper function to initalize worker threads. Safely handling worker
 *        thread termination
 *
 * @params threads number of threads specified
 * @params workers unique worker thread identifier
 *
 * @returns 0 upon succeesful creation of threads, -1 if error
 */
int init_worker_threads(int threads, pthread_t *workers);

#endif
