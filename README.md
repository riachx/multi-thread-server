
# Assignment 4

Combines Assignments 2 and 3 to build a multi-threaded HTTP server. Namely,
this server adds a thread-safe queue and some rwlocks (Asgn 3) to an HTTP server (Asgn 2) so
that the server can serve multiple clients simultaneously. Ensures that its responses conform
to a coherent and atomic linearization of the client requests. Creates an audit log that identifies the linearization of the server.

## Build

    $ make

## Format

    $ make format

## Clean

    $ make clean

## Files

#### httpserver.c

Implementation for the Multithreaded HTTP Server. Implements a hash table to associate a unique lock for each URI. Sends an audit log to ensure linearization.

#### asgn2_helper_funcs.h

Header file for asgn4_helper_funcs.a

#### asgn4_helper_funcs.a

Contains all of the implementation for the header files below.

#### queue.h

Header file for queue implementation

#### response.h

Header file for managing responses

#### debug.h

Header file for debugging

#### connection.h

Header file for managing connections

#### request.h

Header file for managing requests

#### rwlock.h

Header for reader/writer lock implementation

#### Makefile

Makefile contains the commands to compile, clean, and format the file.


## Notes:

#### 11/24
    A lot of code is given, Mitchell mentions that u can grab a hashing function from online since that's not the point of the asgn.
    Added basic getopt stuff and error handling for get/put.

#### 11/26
    Go to Vincent section -->
    How to globalize?
    Where am i adding to the hash table?
    Hash table conceptually
    I don't understand the mutex?
    How to initialize threads properly?
    In get, how do we "create a rwlock associated with the uri if it does not exist"

#### 11/27
    Hash table is causing issues? Is it to do with how im initializing the threads?
    Seg faulting. I might be accessing a node that doesn't exist
    I finally figured out the logic for if/else. 
    never give up girlie <33 fix ur hash table so ur server can be more efficient!!!

#### 11/28
    Remember to close your connection once. Program was only working with one thread because you did closed it twice.
    You don't need to allocate memory explicitly for the threadobj, you can just create an object.
    Tested hash table seperately and it worked fine. Issue was the connection. Single threaded servers handle it diff.
    I also finally understand the global mutex. you need rwlock as well as a global mutex since a global mutex will allow for one at a time access to certain parts of the get/put. 
    Audit log super simple, just one line.
