/*
 * Swarthmore College, CS 87
 * Copyright (c) 2020 Swarthmore College Computer Science Department,
 * Swarthmore PA, Professor Tia Newhall
 * 
 * SLY: shared.c
 * Authors: Sasha Casada, Yatin Lala, and Leo Douhovnikoff (12-18-2023)
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include <sys/time.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>

#include <stdarg.h> // for our logging function

#include "shared.h"

extern log_info_t logger;

void log_record(const char* format, ... ) {
  float ms;
  gettimeofday(&(logger.cur_tv),NULL);

  ms = (logger.cur_tv.tv_sec - logger.start_tv.tv_sec)*1000;
  ms += ((float)logger.cur_tv.tv_usec - (float)logger.start_tv.tv_usec)/1000;

  va_list args;
  fprintf(logger.log_file,"[%6.2f]  ",ms);//,time_len,1,log->log_file);
  va_start( args, format );
  vfprintf( logger.log_file, format, args );
  va_end( args );

  fflush(logger.log_file);
}

void host_connection(int portnum, int *listenfd, struct sockaddr_in *caddr, unsigned int *socklen)
{
  struct sockaddr_in saddr;
  struct linger linger_val;
  int ret, optval;

  // step (1) create a TCP socket
  *listenfd = socket(AF_INET, SOCK_STREAM, 0);
  if (*listenfd == -1) {
    perror("ERROR: create socket failed.\n");
    exit(1);
  }
  // STEP (1.a) set socket opts on the listen socket SO_LINGER off
  //    and SO_REUSEADDR here is the code to do it (it is complete)
  linger_val.l_onoff = 0;
  linger_val.l_linger = 0;
  ret = setsockopt(*listenfd, SOL_SOCKET, SO_LINGER, (void *)&linger_val,
                   (socklen_t)(sizeof(struct linger)));
  if (ret < 0) {
    perror("ERROR: setting socket option SO_LINGER failed.\n");
    exit(1);
  }

  // STEP (1.b) set SO_REUSEADDR on a socket to true (1):
  optval = 1;
  ret = setsockopt(*listenfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
  if (ret < 0) {
    perror("ERROR: setting socket option SO_REUSEADDR failed.\n");
    exit(1);
  }

  // STEP (2) bind to port
  saddr.sin_family = AF_INET;
  saddr.sin_port = htons(portnum);
  saddr.sin_addr.s_addr = INADDR_ANY;

  // STEP (3) call bind
  ret = bind(*listenfd, (struct sockaddr *)&saddr, sizeof(saddr));
  if (ret < 0) {
    perror("ERROR: binding to port failed.\n");
    exit(1);
  }
  
  // STEP (4) tell OS we are going to listen on this socket
  ret = listen(*listenfd, BACKLOG);
  if (ret < 0) {
    perror("ERROR: listening on socket failed.\n");
    exit(1);
  }
  *socklen = (unsigned int)sizeof(caddr);
}

void get_chunk_states(struct UsageInfo *info, char *file_path) {
  struct InfoDictionary *info_dict = info->info_dict;
  int offset_start = 0; 
  int offset_end = info_dict->chunk_size;
  char command[2048];
  char sha256sum_output[65];
  char sha256sum_witness[65];
  struct stat st = {0};
  if (stat(file_path, &st) == -1) {
    log_record("Path '%s' does not exist\n", file_path);
    FILE *file = fopen(file_path, "w");
    if (file == NULL) {
      log_record("File '%s' creation error. Exiting\n", info_dict->file_name);
      fprintf(stderr, "Error (%d): %s\n", errno, strerror(errno));
    }
  }
  for (int i=0; i<info_dict->chunk_total; i++) {
    if (i != 0) {
      offset_start = (info_dict->chunk_size*i)+1; 
    }
    if (i == info_dict->chunk_total-1) {
      offset_end = (info_dict->file_size - ( (info_dict->chunk_total-1)* info_dict->chunk_size) );
      //printf("NOTE: I am changing offset!\n");
    }
    sprintf(command, "tail -c +%d %s | head -c %d | sha256sum | awk '{ print $1 }'", 
      offset_start, file_path, offset_end);
    //printf("offset_start: %d; offset_end: %d\n", offset_start, offset_end);
    FILE *fp = popen(command, "r");
    if (fp == NULL) {
      perror("Error while hashing file");
      exit(EXIT_FAILURE);
    }
    fgets(sha256sum_output, 65, fp);
    pclose(fp);
    sprintf(sha256sum_witness, "%.64s", &info_dict->chunks[i*64]);
    if (validate_sha256sum(sha256sum_witness, sha256sum_output) == 0) {
      info->chunk_states[i] = 1;
    }
    else {
      info->chunk_states[i] = 0;
    }
  }
}

void sha256sum(char *path_to_file, char *sha256sum_output) {
    char command[256];
    sprintf(command, "sha256sum %s | awk '{ print $1 }'", path_to_file);
    FILE *fp = popen(command, "r");
    if (fp == NULL) {
        perror("Error while hashing file");
        exit(EXIT_FAILURE);
    }
    fgets(sha256sum_output, 65, fp);
    pclose(fp);
}

int validate_sha256sum(char *sha256_checksum1, char* sha256_checksum2) {
    //char sha256_hash[65];
    //sha256sum(path_to_file, sha256_hash);
    // printf("We are comparing:\n\t%s\n\t%s\n\n", sha256_checksum1, sha256_checksum2);
    return strcmp(sha256_checksum1, sha256_checksum2);
}

void print_info_dictionary(struct InfoDictionary *info_dict)
{
    printf("file_path: %s\n", info_dict->file_path);
    printf("file_name: %s\n", info_dict->file_name);
    printf("file_size: %d\n", info_dict->file_size);
    printf("sha256sum: %s\n", info_dict->sha256sum);
    printf("chunk_size: %d\n", info_dict->chunk_size);
    printf("chunk_total: %d\n", info_dict->chunk_total);
    printf("chunks: %s\n", info_dict->chunks);
}