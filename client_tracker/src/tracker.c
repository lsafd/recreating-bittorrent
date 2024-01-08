/*
 * Swarthmore College, CS 87
 * Copyright (c) 2020 Swarthmore College Computer Science Department,
 * Swarthmore PA, Professor Tia Newhall
 * 
 * SLY: tracker.c
 * Authors: Sasha Casada, Yatin Lala, and Leo Douhovnikoff (12-18-2023)
 */

#include <errno.h>
#include <pthread.h>
#include <readline/history.h>
#include <readline/readline.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include "shared.h"
#include "hashtable.h"

////////////////////////////// DEFINITIONS ////////////////////////////////////

#define MAX_CONNECTIONS         200     // max # of connections for server
#define MAX_FILES               20      // max # of files tracker can store

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
  pthread_t *pthread_id; // current pthread id
};

/* global variable that encapsulates an array which contains all
 * client data for clients that are currently connected to the
 * server for index clients[MAX_CONNECTION]
 */
struct client clients[MAX_CONNECTIONS];

int connected_clients;  
char **files;
log_info_t logger;

////////////////////////// Function Prototypes ////////////////////////////////

/* initializes a clients thread */
void thread_init(struct client *client, int ntids);

/* function for created threads to manage a single cli_con */
void *client_connect(void *args);

/* handles file request from connected client */
void *handle_file_request(void *args);

/* handles add request from connected client */
void *handle_seed_request(void *args);

/* handles add request from connected client */
void *handle_add_request(void *args);

///////////////////////////////////////////////////////////////////////////////

/**
 * NOTE: We have (2) mystery memory leaks in tracker. I think one (or both)
 * are related to the expected memory leak from lab 2, although I am not
 * entirely sure. One of them is in some library __libc_unwind_link_get,
 * and the other is due to our detatched p_thread (which does clean up).
 * 
 * Realistically, the seeder IPs need to be held inside of some dynamic
 * database which is read by the server which routine polling for active/
 * inactive peers. The hashtable cannot be gracefully destroyed in this
 * implementation, sadly. Hard to handle in C. Requires more effort.
 **/

int main(int argc, char **argv) 
{
  int listenfd, sockfd;
  struct sockaddr_in caddr;
  struct client *newClient;
  unsigned int socklen;
  tsize_t send_tag;

  /* initialize log_file and log start time */
  logger.log_file = fopen("tracker.log","w");
    gettimeofday(&(logger.start_tv),NULL);

  host_connection(P2T_PORTNUM, &listenfd, &caddr, &socklen);

  connected_clients = 0;
  // (1) the server is now ready to accept connections from clients while the 
  //     server is running, add new connections <= 5.
  while (1) {

    // (2) check if new connection exceeds MAX_CONNECTIONS, and if so, we want 
    //     to send HELLO_ERROR to the client. Or accept.
    newClient = NULL;
    sockfd = accept(listenfd, (struct sockaddr *)&caddr, &socklen);

    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(caddr.sin_addr), client_ip, INET_ADDRSTRLEN);
    log_record("(%s) Client attempting to connect...\n", client_ip);

    for (int i = 0; i < MAX_CONNECTIONS; i++) {
      if (clients[i].isActive == 0) { // only set newClient to nonActive
        newClient = &clients[i];      // member of clients
        break;
      }
    }
    if (newClient == NULL) {
      send(sockfd, &send_tag, sizeof(tsize_t), MSG_NOSIGNAL);
      fprintf(stderr, "ERROR: client (%s) attempted to connect when "
             "MAX_CONNECTIONS has been reached.\n", client_ip);
    } else {
      newClient->sockfd = sockfd;
      newClient->isActive = 1;
      strcpy(newClient->ip, client_ip);
      if (newClient->sockfd == -1) {
        fprintf(stderr, "ERROR: client (%s) failed to accept.\n", client_ip);
        break;
      }
      log_record("(%s) Accepted new client socket.\n", client_ip);
      printf("(%s) Client connected.\n", client_ip);
      // (3) init client thread for new connection
      thread_init(newClient, connected_clients);
      connected_clients++;
    }
  }
  close(listenfd);
}

void *handle_file_request(void *args)
{
  tsize_t send_tag;
  struct thread_info *t_info;
  char file_request[65]; // must include null-terminating \0
  
  t_info = (struct thread_info *)args;
  send_tag = REQUEST_APPROVED;
  send(clients[t_info->id].sockfd, &send_tag, sizeof(tsize_t), 
    MSG_NOSIGNAL);
  recv(clients[t_info->id].sockfd, &file_request, sizeof(char)*65, 
    MSG_NOSIGNAL);
  struct nlist *lookup = hash_lookup(file_request);

  if (lookup != NULL) { // if file is in file hash, accept
    printf("(%s) Serving request of '(%.8s...)' on the network with"
      " (%d) peers.\n", 
      clients[t_info->id].ip, lookup->name, lookup->defn->curr_peers);
    send_tag = REQUEST_FOUND;
    send(clients[t_info->id].sockfd, &send_tag, sizeof(tsize_t), MSG_NOSIGNAL);
    send(clients[t_info->id].sockfd, &lookup->defn->curr_peers, sizeof(int), MSG_NOSIGNAL);
    for (int i = 0; i < lookup->defn->curr_peers; i++) {
      send(clients[t_info->id].sockfd, lookup->defn->peers[i], INET_ADDRSTRLEN, MSG_NOSIGNAL);
    }
    return NULL;
  }
  else { // if file is not in hash, reject
    send_tag = REQUEST_NOT_FOUND;
    send(clients[t_info->id].sockfd, &send_tag, sizeof(tsize_t), MSG_NOSIGNAL);
    printf("(%s) Attempted to request a file not on the network!\n", 
      clients[t_info->id].ip);
    return NULL;
  } 
}

void *handle_seed_request(void *args)
{
  tsize_t send_tag;
  struct thread_info *t_info;
  char file_request[65]; // must include null-terminating \0
  
  t_info = (struct thread_info *)args;
  send_tag = SEED_APPROVED;

  send(clients[t_info->id].sockfd, &send_tag, sizeof(tsize_t), MSG_NOSIGNAL);
  recv(clients[t_info->id].sockfd, &file_request, sizeof(char)*65, MSG_NOSIGNAL);

  struct nlist *lookup = hash_lookup(file_request);
  if (lookup != NULL) { // if file is in file hash, accept
    lookup->defn->peers[lookup->defn->curr_peers] = clients[t_info->id].ip;
    lookup->defn->curr_peers += 1;
    send_tag = SEED_SUCCESS;
    send(clients[t_info->id].sockfd, &send_tag, sizeof(tsize_t), MSG_NOSIGNAL);
    printf("(%s) Now seeding file (%.8s...) in peerswarm of (%d).\n", 
      clients[t_info->id].ip, file_request, lookup->defn->curr_peers);
    return NULL;
  }
  else { // if file is not in hash, reject
    struct peer_info *p_info;
    p_info = malloc(sizeof(struct peer_info));
    p_info->curr_peers = 0;
    hash_install(file_request, p_info);
    struct nlist *result = hash_lookup(file_request);
    result->defn = p_info;  // this is very jimmy-rigged. should be set 
                            // correctly the first time??
    printf("(%s) Added new file: '(%.8s...)' to the network.\n", 
      clients[t_info->id].ip, file_request);
    p_info->peers[p_info->curr_peers] = clients[t_info->id].ip;
    p_info->curr_peers += 1;
    send_tag = SEED_SUCCESS;
    send(clients[t_info->id].sockfd, &send_tag, sizeof(tsize_t), MSG_NOSIGNAL);
    printf("(%s) Now seeding file (%.8s...) in peerswarm of (%d).\n", 
      clients[t_info->id].ip, file_request, p_info->curr_peers);
    return NULL;
  } 
}

void *handle_add_request(void *args)
{
  tsize_t send_tag;
  struct thread_info *t_info;
  struct peer_info *p_info;
  p_info = malloc(sizeof(struct peer_info));
  char file_request[65]; // must include null-terminating \0
  
  t_info = (struct thread_info *)args;
  send_tag = ADD_APPROVED;

  send(clients[t_info->id].sockfd, &send_tag, sizeof(tsize_t), MSG_NOSIGNAL);
  recv(clients[t_info->id].sockfd, &file_request, sizeof(char)*65, MSG_NOSIGNAL);

  if (hash_lookup(file_request) != NULL) { // if file is in file hash, reject
    send_tag = ADD_FAIL;
    send(clients[t_info->id].sockfd, &send_tag, sizeof(tsize_t), MSG_NOSIGNAL);
    printf("(%s) Attempted to add file already on network!\n", 
      clients[t_info->id].ip);
    return NULL;
  } 
  else { // add our new file to our file hash
    p_info->curr_peers = 0;
    hash_install(file_request, p_info);
    struct nlist *result = hash_lookup(file_request);
    result->defn = p_info;  // this is very jimmy-rigged. should be set 
                            // correctly the first time??
    send_tag = ADD_SUCCESS;
    send(clients[t_info->id].sockfd, &send_tag, sizeof(tsize_t), MSG_NOSIGNAL);
    printf("(%s) Added new file: '(%.8s...)' to the network.\n", 
      clients[t_info->id].ip, file_request);
    return NULL;
  }
}

void thread_init(struct client *client, int ntids) 
{
  pthread_t *tid;
  int ret;
  struct thread_info *tid_info;

  tid = malloc(sizeof(pthread_t));
  if (!tid) { 
    perror("ERROR: malloc(pthread_t) failed.");
    exit(1); }

  tid_info = malloc(sizeof(struct thread_info));
  if (!tid_info) {  
    perror("ERROR: malloc(tid_info) failed.");
    exit(1); }
  tid_info->id = ntids;
  tid_info->ntids = ntids;
  tid_info->pthread_id = tid;

  ret = pthread_create(tid, NULL, client_connect, tid_info);
  if (ret) {
    perror("ERROR: pthread_create() failed.");
    exit(1); }
  pthread_detach(*tid);
}

void *client_connect(void *args) 
{
  tsize_t send_tag, recv_tag;
  struct thread_info *t_info;

  t_info = (struct thread_info *)args;

  // Client handshake
  recv(clients[t_info->id].sockfd, &recv_tag, 1, 0);
  send_tag = HANDSHAKE_OK;
  if (recv_tag != HANDSHAKE) {
    send_tag = HANDSHAKE_ERROR;
    perror("ERROR: failed to make client-server handshake.\n"); 
  }
  send(clients[t_info->id].sockfd, &send_tag, sizeof(tsize_t),
             MSG_NOSIGNAL);
  char client_ip[INET_ADDRSTRLEN];

  // STEP (5) update struct clients[] with new client information
  strcpy(client_ip, clients[t_info->id].ip);

  log_record("(%s) Shook hands with client.\n", client_ip);

  /* This begins the recv/send client-server broadcast loop */
  // STEP (1) recv message from server
  if (recv(clients[t_info->id].sockfd, &recv_tag, sizeof(tsize_t), 0) == -1) {
    perror(strerror(errno));
    goto end;
  }

  // STEP (1.a) recv RECV_TAG (1) byte tag
  switch (recv_tag) {
    case QUIT:
        send_tag = QUIT;
        send(clients[t_info->id].sockfd, &send_tag, sizeof(tsize_t),
          MSG_NOSIGNAL);
        break;

    case ADD_REQUEST:
        printf("(%s) Client usage_mode: ADD_REQUEST\n", client_ip);
        handle_add_request(t_info);
        break;

    case SEED_REQUEST:
        printf("(%s) Client usage_mode: SEED_REQUEST\n", client_ip);
        handle_seed_request(t_info);
        break;

    case FILE_REQUEST:
        printf("(%s) Client usage_mode: FILE_REQUEST\n", client_ip);
        handle_file_request(t_info);
        break;

    default:
        fprintf(stderr, "ERROR: recieved unexpected tag (%d) from client.\n", recv_tag);
        break;
    }

end: 
  /* Make sure to free up at the end! */
  free(t_info->pthread_id);
  free(t_info);
  pthread_exit(NULL);
}
