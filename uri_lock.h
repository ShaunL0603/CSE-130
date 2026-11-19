#ifndef URI_LOCK_H
#define URI_LOCK_H

#include "rwlock"

/**
 * @struct uri_node_t
 * 
 * @brief typdef renames struct uri_node. Storage for each request
 */
typedef struct uri_node uri_node_t;

/**
 * @brief Initializes global URI lock hash table nd protecting mutex
*/
void uri_lock_init(void);

/**
 * @brief Retrieves the rwlock_t associated with the given URI, creating
 *        and inserting a new entry if one doesn't already exist
 * 
 * @param uri Null-terminated URI string
 * @return Pointer to the URI's rwlock_t, or NULL on allocation failure
 */
rwlock_t *uri_lock_get(const char *uri);

/**
 * @brief Frees all nodes, URI strings, rwlocks, and destryos table mutex
 */
void uri_lock_destroy(void);

#endif