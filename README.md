
# Multithreaded HTTP Server

Builds a multi-threaded HTTP server. This server adds a thread-safe queue and some rwlocks to an HTTP server so that the server can serve multiple clients simultaneously. Ensures that its responses conform to a coherent and atomic linearization of the client requests. Creates an audit log that identifies the linearization of the server.

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

