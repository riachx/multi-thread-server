// Asgn 4: A Multithreaded HTTP Server
// Single threaded HTTP Server By:
//     Eugene Chou
//     Andrew Quinn
//     Brian Zhao
// Multithreaded HTTP Server By
//     Ria Chockalingam
// Hashing function hash() by
//     Dan Berstein
//     https://theartincode.stanis.me/008-djb2/

#include "asgn2_helper_funcs.h"
#include "connection.h"
#include "response.h"
#include "debug.h"
#include "queue.h"
#include "rwlock.h"
#include "request.h"

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <sys/stat.h>
#include <pthread.h>

// Node Struct
struct rwlockHTNode;

typedef struct rwlockHTNode {
    char *uri;
    rwlock_t *rwlock;
    struct rwlockHTNode *next;
} rwlockHTNode;

// Struct for hash table filled with buckets
typedef struct HT {
    int num_buckets;
    struct rwlockHTNode **buckets;

} HT;

// Thread struct
typedef struct ThreadObj {
    pthread_t thread;
    HT *ht;
    pthread_mutex_t *table_mutex;
} ThreadObj;

pthread_mutex_t mutex;

// global hash table
// To keep track of fd for locking. Each file should get a rwlock so you need something to manage that
HT *ht;
ThreadObj threads;
queue_t *q; // globalize queue to push connection to

void handle_connection(int);
void handle_get(conn_t *);
void handle_put(conn_t *);
void handle_unsupported(conn_t *);

// worker thread
void *process_connection() {
    while (1) {
        uintptr_t connfd = 0;
        queue_pop(q, (void **) &connfd);
        handle_connection(connfd);
        close(connfd);
    }
}

int main(int argc, char **argv) {

    // if wrong number of arguments
    // thread number is optional!
    if (argc < 2) {
        warnx("wrong arguments: %s port_num", argv[0]);
        fprintf(stderr, "usage: %s [-t thread] <port>\n", argv[0]);
        return EXIT_FAILURE;
    }

    long num_threads_long = 4; // default number of threads
    char *end_ptr_threads;
    int opt;

    while ((opt = getopt(argc, argv, "t:")) != -1) {
        switch (opt) {
        case 't':
            num_threads_long = strtol(optarg, &end_ptr_threads, 10);

            // error handling to see if num_threads is valid
            if ((errno == ERANGE && (num_threads_long == LONG_MAX || num_threads_long == LONG_MIN))
                || (errno != 0 && num_threads_long == 0)) {
                fprintf(stderr, "Conversion error occurred, or the input is out of range.\n");
                exit(EXIT_FAILURE);
            } else if (end_ptr_threads == optarg) {
                fprintf(stderr, "No digits were found in the input.\n");
                exit(EXIT_FAILURE);
            } else if (*end_ptr_threads != '\0') {
                fprintf(stderr, "Non-integer input found: %c\n", *end_ptr_threads);
                exit(EXIT_FAILURE);
            }

            break;
        default: fprintf(stderr, "Usage: %s [-t threads] <port>\n", argv[0]); exit(EXIT_FAILURE);
        }
    }

    // convert long to int
    int num_threads = (int) num_threads_long;

    // leftover argument should be port
    if (optind >= argc) {
        fprintf(
            stderr, "Expected argument after options\nUsage: %s [-t threads] <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // ensure valid port number
    char *end_ptr_port = NULL;
    size_t port = (size_t) strtoull(argv[optind], &end_ptr_port, 10);
    if (end_ptr_port && *end_ptr_port != '\0') {
        warnx("invalid port number: %s", argv[1]);
        return EXIT_FAILURE;
    }

    signal(SIGPIPE, SIG_IGN);
    Listener_Socket sock;
    listener_init(&sock, port);

    //initialize queue
    q = queue_new(num_threads);

    // initialize global mutex
    pthread_mutex_init(&mutex, NULL);

    // create a hashtable or linkedlist to store rwlock (can be global, passed into arg to pthread create )
    threads.ht = (HT *) malloc(sizeof(HT));
    threads.ht->num_buckets = 5000; // default num of buckets
    threads.ht->buckets = (struct rwlockHTNode **) malloc(
        sizeof(struct rwlockHTNode *) * (threads.ht->num_buckets));

    // populate the buckets
    for (int i = 0; i < threads.ht->num_buckets; i++) {
        threads.ht->buckets[i] = NULL;
    }

    // create t worker threads
    for (int i = 0; i < num_threads; i++) {
        pthread_create(&threads.thread, NULL, process_connection, (void *) &threads);
    }

    while (1) {
        int connfd = listener_accept(&sock);
        queue_push(q, (void *) (size_t) connfd); // push connection to queue (queue can be global)
    }

    return EXIT_SUCCESS;
}

// handles connection
void handle_connection(int connfd) {

    conn_t *conn = conn_new(connfd);

    const Response_t *res = conn_parse(conn);

    if (res != NULL) {
        conn_send_response(conn, res);
    } else {
        //debug("%s", conn_str(conn));
        const Request_t *req = conn_get_request(conn);
        if (req == &REQUEST_GET) {
            handle_get(conn);
        } else if (req == &REQUEST_PUT) {
            handle_put(conn);
        } else {
            handle_unsupported(conn);
        }
    }

    conn_delete(&conn);
}

// handling unsupported request
void handle_unsupported(conn_t *conn) {
    conn_send_response(conn, &RESPONSE_NOT_IMPLEMENTED);
}

// credit Dan Bernstein for the hash function
// https://theartincode.stanis.me/008-djb2/
unsigned long hash(char *str) {
    unsigned long hash = 5381;
    int c;
    while (*str != '\0') {
        c = *str;
        hash = ((hash << 5) + hash) + (unsigned char) c;
        str++;
    }
    return hash;
}

// Returns the lock associated with the uri
// If it does not exist, it creates one
rwlock_t *handle_uri(char *uri, HT *table) {

    size_t index = hash(uri) % (table->num_buckets);
    rwlockHTNode *list = (table->buckets)[index];

    while (list && strcmp(list->uri, uri) != 0) {
        list = list->next;
    }
    if (list != NULL) { // already exists
        //printf("URI Exists: Returning lock\n");
        return list->rwlock;
    } else { // does not exist
        rwlockHTNode *new_node = malloc(sizeof(rwlockHTNode));
        if (new_node == NULL) {
            // handle memory allocation failure
            exit(EXIT_FAILURE);
        }

        rwlock_t *rwlocker = rwlock_new(N_WAY, 1);
        new_node->rwlock = rwlocker;
        new_node->uri = strdup(uri);
        new_node->next = (table->buckets)[index];
        (table->buckets)[index] = new_node;
        //printf("URI DNE: Created lock\n");
        return new_node->rwlock;
    }
}

// handles get requests
void handle_get(conn_t *conn) {

    pthread_mutex_lock(&mutex); // lock global mutex
    char *uri = conn_get_uri(conn);

    int fd = open(uri, O_RDONLY);
    rwlock_t *rwlock = handle_uri(uri, threads.ht); // lock reader lock
    reader_lock(rwlock);

    pthread_mutex_unlock(&mutex); // unlock global mutex

    // 1. Open the file.
    // If  open it returns < 0, then use the result appropriately
    //   a. Cannot access -- use RESPONSE_FORBIDDEN
    //   b. Cannot find the file -- use RESPONSE_NOT_FOUND
    //   c. other error? -- use RESPONSE_INTERNAL_SERVER_ERROR
    // (hint: check errno for these cases)!

    const Response_t *res = NULL;

    if (fd < 0) {
        if (errno == EACCES) {
            // Cannot access the file
            res = &RESPONSE_FORBIDDEN;

        } else if (errno == ENOENT) {
            // Cannot find the file
            res = &RESPONSE_NOT_FOUND;
        } else {
            // Other error occurred
            res = &RESPONSE_INTERNAL_SERVER_ERROR;
        }
        conn_send_response(conn, res);
    }

    // 2. Get the size of the file.
    struct stat file_stat;

    // Handle error if fstat() fails
    if (fstat(fd, &file_stat) != 0) {
        res = &RESPONSE_INTERNAL_SERVER_ERROR;
    }

    off_t file_size = file_stat.st_size;

    // 3. Check if the file is a directory, because directories *will*
    // open, but are not valid.
    if (S_ISDIR(file_stat.st_mode)) {
        res = &RESPONSE_FORBIDDEN;
    }

    if (fd >= 0) {
        res = conn_send_file(conn, fd, file_size); // 4. Send the file
    }

    if (res == NULL) {
        res = &RESPONSE_OK;
    }

    fprintf(stderr, "GET,%s,%hu,%s\n", uri, response_get_code(res),
        conn_get_header(conn, "Request-Id")); // audit log
    close(fd);
    reader_unlock(rwlock); // unlock reader lock
}

// handles put requests
void handle_put(conn_t *conn) {
    pthread_mutex_lock(&mutex); // lock global mutex
    char *uri = conn_get_uri(conn); // store uri

    rwlock_t *rwlock = handle_uri(uri, threads.ht); // get lock associatd with uri
    writer_lock(rwlock); // lock writer lock

    // check if file already exists before opening it.
    bool existed = access(uri, F_OK) == 0;
    const Response_t *res = NULL;

    // Open the file..
    int fd = open(uri, O_CREAT | O_TRUNC | O_WRONLY, 0600);
    pthread_mutex_unlock(&mutex);

    if (fd < 0) {
        if (errno == EACCES || errno == EISDIR) {
            res = &RESPONSE_FORBIDDEN;
        } else if (errno == ENOENT) {
            res = &RESPONSE_NOT_FOUND;
        } else {
            res = &RESPONSE_INTERNAL_SERVER_ERROR;
        }
        conn_send_response(conn, res);
        close(fd);
        return;
    }

    res = conn_recv_file(conn, fd);

    if (res == NULL && existed) {
        res = &RESPONSE_OK;
    } else if (res == NULL && !existed) {
        res = &RESPONSE_CREATED;
    }

    fprintf(stderr, "PUT,%s,%hu,%s\n", uri, response_get_code(res),
        conn_get_header(conn, "Request-Id")); // audit log
    writer_unlock(rwlock); // unlock writer lock
    close(fd);
    conn_send_response(conn, res);
}
