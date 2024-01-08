/*
 * Swarthmore College, CS 87
 * Copyright (c) 2020 Swarthmore College Computer Science Department,
 * Swarthmore PA, Professor Tia Newhall
 * 
 * SLY: peer.h
 * Authors: Sasha Casada, Yatin Lala, and Leo Douhovnikoff (12-18-2023)
 */

#ifndef _PEER_H_
#define _PEER_H_

#include "shared.h"

#define BACKLOG             12      // backlog length for listen_socket
#define MAX_CONNECTIONS     256      // max # of connections for server  
#define MAX_FILENAME        1000    // max size of an allowed filename + dir

/* this struct represents all the data which encapsulates a single
 * user that may be connected to our messaging server at a given
 * moment of time
 */
struct client {
  int isActive;             // whether or not client is active
  int sockfd;               // socket file descriptor location
  char ip[INET_ADDRSTRLEN]; // ip of client
  char msg_buffer[BUFSIZE]; // message buffer per client
};

/* struct that encapsulates all of the information that is to be
 * passed to each thread during thread creation
 */
struct thread_info {
  int id;                // individual thread id
  int ntids;             // total active threads
  struct UsageInfo *seed_info;
  pthread_t *pthread_id; // current pthread id
};

/* global variable that encapsulates an array which contains all
 * client data for clients that are currently connected to the
 * server for index clients[MAX_CONNECTION]
 */
extern struct client clients[MAX_CONNECTIONS];

/* total number of currently connected clients */
extern int connected_clients;

/* function that creates threads for cord. of recv/send */
void thread_init(struct client *client, struct UsageInfo *seed_info, 
  int ntids);

/* helper function for thread of t_main which manages send */
void *tracker_connect();

/* helper function for thread of t_main which manages send */
void *client_send(void *args);

/* helper function for thread of t_main which manages recv */
void *client_recv(void *args);

// /* TODO write a great comment */
// void *thread_provide(void* args);

/* initialize a connection with a leeching peer, provide chunks */
void seed_provide(struct UsageInfo *seed_info);

#endif