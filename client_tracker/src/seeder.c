/*
 * Swarthmore College, CS 87
 * Copyright (c) 2020 Swarthmore College Computer Science Department,
 * Swarthmore PA, Professor Tia Newhall
 * 
 * SLY: seeder.c
 * Authors: Sasha Casada, Yatin Lala, and Leo Douhovnikoff (12-18-2023)
 */

/**
 * Resources used in our networking implementation:
 *
 * https://stackoverflow.com/questions/1271064/how-do-i-loop-through-all-files-in-a-folder-using-c
 * https://man7.org/linux/man-pages/man2/sendfile.2.html
 * https://stackoverflow.com/questions/11720079/linux-command-to-get-size-of-files-and-directories-present-in-a-particular-folde
 * https://www.reddit.com/r/learnprogramming/comments/2w0m1j/google_says_a_kilobyte_is_1000_bytes_is_it_1024/
 * https://stackoverflow.com/questions/11952898/c-send-and-receive-file
 **/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/sendfile.h>
#include <ctype.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <pthread.h>
#include <getopt.h>
#include <math.h>
#include <fcntl.h>
#include "shared.h"
#include "seeder.h"

////////////////////////////// DEFINITIONS ////////////////////////////////////

struct client clients[MAX_CONNECTIONS];
int connected_clients;  

///////////////////////////////////////////////////////////////////////////////

void *provide_chunkset_to_peer(void *args) 
{
  tsize_t send_tag, recv_tag;
  struct thread_info *t_info;

  t_info = (struct thread_info *)args;
  struct UsageInfo *seed_info = t_info->seed_info;

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

  log_record("(%s) Shook hands with new client.\n", client_ip);

  // NOTE: Files are in KiB 128*(1024), not 128*(1000).
  int sent_bytes = 0;
  struct stat file_stat;
  off_t offset;
  long int remain_data;

  char *p_upload_path = seed_info->upload_path;
  char *p_filename = seed_info->info_dict->file_name;
  char upload_file_path[MAX_FILENAME];

  int file_size = seed_info->info_dict->file_size;
  int chunk_size = seed_info->info_dict->chunk_size; 
  int chunk_total = seed_info->info_dict->chunk_total; 
  int last_chunk_size = file_size - (chunk_size * (chunk_total-1) );
  
  int *p_chunk_states = seed_info->chunk_states;
  int *p_requested_chunks = malloc(sizeof(int) * chunk_total);

  sprintf(upload_file_path , "%s/%s", p_upload_path, p_filename);

  struct stat st = {0};
  if (stat(p_upload_path, &st) == -1) {
    log_record("Path '%s' does not exist. Exiting\n", p_upload_path);
    printf("Path '%s' does not exist. Exiting\n", p_upload_path);
    fprintf(stderr, "Error (%d): %s\n", errno, strerror(errno));
    exit(1);
    }

  get_chunk_states(seed_info, upload_file_path);
  for (int i=0; i<chunk_total; i++) {
    // printf(" %d", p_chunk_states[i]);
  } 

  // printf("Providing piece_states: [");
  for (int i = 0; i < chunk_total-1; i++) {
    // printf("%d, ", p_chunk_states[i]);
  }
  // printf("%d]\n", p_chunk_states[chunk_total-1]);

  /* Send list of present chunks */ 
  send(clients[t_info->id].sockfd, p_chunk_states, sizeof(int) * 
    chunk_total, MSG_NOSIGNAL);
  // printf("Sending chunks...\n");

  /* recieve the list of wanted chunks */
  recv(clients[t_info->id].sockfd, p_requested_chunks, sizeof(int) * 
    chunk_total, MSG_WAITALL);

  // printf("Requested piece_states: [");
  for (int i = 0; i < chunk_total-1; i++) {
    // printf("%d, ", p_requested_chunks[i]);
  }
  // printf("%d]\n", p_requested_chunks[chunk_total-1]);
  
  int file;
  /* open our file for upload */
  file = open(upload_file_path, O_RDONLY);
  if (file == -1) {
    fprintf(stderr, "Could not open file for seeding. %s", strerror(errno));
    return NULL; // exit and destory objects
  }
  // printf("We have opened file...!\n");



  /* Get file stats */
  if (fstat(file, &file_stat) < 0) {
    fprintf(stderr, "Error fstat. Could not read file. %s", strerror(errno));
    return NULL; // exit and destory objects
  }
  if (file_stat.st_size != file_size) {
    fprintf(stderr, "Bad seed. File size not correct. %s", strerror(errno));
    return NULL; // exit and destory objects
  }

  long int chunk_size_long = (long int)chunk_size;
  long int last_chunk_size_long = (long int)last_chunk_size;

  /* Sending chunks loop */
  for (int i = 0; i<chunk_total; i++) {

    if (p_requested_chunks[i] != 1) {
      // printf("DEBUG: Client does not want chunk_id: %d!\n", i);
      continue;
    }

    /* sending chunk id */
    int chunk_id = i;
    send(clients[t_info->id].sockfd, &chunk_id, sizeof(int), 0);
    // printf("I am going to send chunk_id: %d.\n", chunk_id);

    /* sending file size */
    if (i == chunk_total-1) {
      send(clients[t_info->id].sockfd, &last_chunk_size_long, 
        sizeof(long int), 0);
      // fprintf(stdout, "File Size: %ld bytes\n", last_chunk_size_long);
    }
    else {
      send(clients[t_info->id].sockfd, &chunk_size_long, sizeof(long int), 0);
      // fprintf(stdout, "File Size: %ld bytes\n", chunk_size_long);
    }

    if (p_chunk_states[chunk_id] != 1) {
      continue;
    }

    remain_data = (long int)chunk_size;
    remain_data = remain_data - 8192;

    if (chunk_id == 0) {
      offset = 0;
      // printf("I have set offset to: %d\n.", chunk_id);
    }
    else {
      offset = chunk_size*chunk_id;
    }
    int total_sent_bytes = 0;
    /* Sending file data */
    while (((sent_bytes = sendfile(clients[t_info->id].sockfd, file, 
      &offset, BUFSIZ)) > 0) && (remain_data > 0)) {
      fprintf(stdout, "1. Server sent %d bytes from file's data, offset is"
        " now : %ld and remaining data = %ld\n", 
        sent_bytes, offset, remain_data);
      remain_data -= (long int)sent_bytes;
      total_sent_bytes += sent_bytes;
      fprintf(stdout, "2. Server sent %d bytes from file's data, offset is" 
        " now : %ld and remaining data = %ld\n", 
        sent_bytes, offset, remain_data);
    }
    // printf("I sent '%d' many bytes!\n", total_sent_bytes);
  }

  if (close(file) == -1) {
    fprintf(stderr, "Error closing file --> %s", strerror(errno));
    return NULL; // exit and destory objects
  }
  // printf("All chunks have been sent.\n");
  return NULL;
}

void thread_init(struct client *client, struct UsageInfo *seed_info, 
  int ntids) 
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
  tid_info->seed_info = seed_info;

  ret = pthread_create(tid, NULL, provide_chunkset_to_peer, tid_info);
  if (ret) {
    perror("ERROR: pthread_create() failed.");
    exit(1); }
  pthread_detach(*tid);
}

void seed_provide(struct UsageInfo *seed_info)
{
  int listenfd, sockfd;
  struct sockaddr_in caddr;
  struct client *newClient;
  unsigned int socklen;
  tsize_t send_tag;

  host_connection(P2P_PORTNUM, &listenfd, &caddr, &socklen);
  connected_clients = 0;

  // STEP (4) the server is now ready to accept connections from clients
  //          while the server is running, add new connections <= 5.
  while (1) {

    // STEP (5) check if new connection exceeds MAX_CONNECTIONS, and if
    //          so, we want to send HELLO_ERROR to the client. Or accept.
    newClient = NULL;
    sockfd = accept(listenfd, (struct sockaddr *)&caddr, &socklen);
    
    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(caddr.sin_addr), client_ip, INET_ADDRSTRLEN);
    // printf("Client connected from IP address: %s\n", client_ip);

    for (int i = 0; i < MAX_CONNECTIONS; i++) {
      if (clients[i].isActive == 0) { // only set newClient to nonActive
        newClient = &clients[i];      // member of clients
        break;
      }
    }
    if (newClient == NULL) {
      send(sockfd, &send_tag, sizeof(tsize_t), MSG_NOSIGNAL);
      perror("ERROR: client attempted to connect when "
             "MAX_CONNECTIONS has been reached.\n");
    } else {
      newClient->sockfd = sockfd;
      newClient->isActive = 1;
      strcpy(newClient->ip, client_ip);
      if (newClient->sockfd == -1) {
        perror("ERROR: client failed to accept.\n");
        break;
      }
      log_record("(%s) Accepted new client socket.\n", newClient->ip);

      // STEP (6) init client thread for new connection
      thread_init(newClient, seed_info, connected_clients);
      connected_clients++;
    }
  }
  close(listenfd);
}
