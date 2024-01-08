/*
 * Swarthmore College, CS 87
 * Copyright (c) 2020 Swarthmore College Computer Science Department,
 * Swarthmore PA, Professor Tia Newhall
 * 
 * SLY: client.c
 * Authors: Sasha Casada, Yatin Lala, and Leo Douhovnikoff (12-18-2023)
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <ctype.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <pthread.h>
#include <getopt.h>
#include <math.h>
#include <dirent.h>
#include "seeder.h"
#include "shared.h"

////////////////////////////// DEFINITIONS ////////////////////////////////////

#define USAGE_NULL        0
#define USAGE_ADD         1
#define USAGE_SEED        2
#define USAGE_REQUEST     3
#define USAGE_GENERATE    4
#define PIECE_LENGTH      128
#define MAX_PATH_LENGTH   1000

log_info_t logger;

int download_from_peerlist(struct UsageInfo *request_info); 

void *download_from_peer(void* args);

void *populate_seeder_info(void* args);

void *download_chunkset_from_peer(void* args);

void generate_file(char *filePath);

/* attempts to request a file on the torrent network */
void request_file(struct UsageInfo *request_info);

/* attempts to seed a file on the torrent network */
void seed_file(struct UsageInfo *seed_info);

/* sends info_dictionary struct in args to server */
void add_file(struct UsageInfo *add_info);

/* downloads from a single peer */
void *download_from_peer(void* args);

/* connect to peer or tracker */
void init_connection(int portnum, int *sockfd, char *ip_addr);

/* attempt a handshake with the tracker */
void tracker_handshake(int sockfd);

/* TODO write a comment */
void init_from_file(struct InfoDictionary *data);

/* parses args for peer client and returns in ArgsInfo struct */
static void parse_args(int ac, char *av[], struct ArgsInfo *data, 
  struct InfoDictionary *info_dict);

/* prints out peer client usage information */
static void usage(void);

///////////////////////////////////////////////////////////////////////////////

int main(int argc, char **argv) 
{
  int sockfd;
  struct ArgsInfo args;
  struct InfoDictionary info_dict;

  /* initialize log_file and log start time */
  logger.log_file = fopen("client.log","w");
    gettimeofday(&(logger.start_tv),NULL);

  parse_args(argc, argv, &args, &info_dict);
  init_from_file(&info_dict);
    // print_info_dictionary(&info_dict);

  init_connection(P2T_PORTNUM, &sockfd, info_dict.tracker_ip);
  tracker_handshake(sockfd);

  switch (args.usage_mode) {
    case USAGE_ADD:
        log_record("usage_mode: USAGE_ADD\n");
        struct UsageInfo add_info;
          add_info.sockfd = sockfd;
          add_info.info_dict = &info_dict;
        add_file(&add_info);
        break;
 
    case USAGE_SEED:
        log_record("usage_mode: USAGE_SEED\n");
        struct UsageInfo seed_info; 
          seed_info.sockfd = sockfd;
          seed_info.upload_path = args.upload_path;
          seed_info.info_dict = &info_dict;
        seed_file(&seed_info);
          seed_info.chunk_states = malloc(sizeof(int)*info_dict.chunk_size);
        seed_provide(&seed_info);
        break;

    case USAGE_REQUEST:
        log_record("usage_mode: USAGE_REQUEST\n");
        struct UsageInfo request_info;
          request_info.sockfd = sockfd;
          request_info.download_dir = args.download_dir;
          request_info.info_dict = &info_dict;
        request_file(&request_info);
          request_info.chunk_states = malloc(sizeof(int)*info_dict.chunk_size);
        while (download_from_peerlist(&request_info) == 1) {
          /* If no valid peers are found, request new peers from tracker */
          tracker_handshake(sockfd);
          request_file(&request_info);
          log_record("DEBUG: Looping download from peerlist\n");
        }
        printf("File '%s' has been downloaded to the current directory!\n", 
          info_dict.file_name);
        break;

    case USAGE_GENERATE:
        log_record("usage_mode: USAGE_GENERATE\n");
        generate_file(args.generate_path);
        break;

    default:
        abort();
    }

  close(sockfd);
  log_record("Closing client-tracker socket...\n");
  log_record("Operation successful. Exiting.\n");

  // free_args_info(&args);
  // free_info_dictonary(&info_dict);
}

int download_from_peerlist(struct UsageInfo *request_info)
{
  struct SeederInfo *seeders = malloc(sizeof(struct SeederInfo) * 
    request_info->num_peers);
  seeders->request_info = request_info;
  pthread_t* threads = malloc(sizeof(pthread_t) * request_info->num_peers);
  int ret;
  int chunk_total = request_info->info_dict->chunk_total;
  int num_peers = request_info->num_peers;
  int filesize = seeders->request_info->info_dict->file_size;
  int *p_chunk_states = request_info->chunk_states;
  char *p_download_dir = seeders->request_info->download_dir;
  char *p_filename = request_info->info_dict->file_name;
  char sha256sum_file[65];

  /* create a file of size of file we will create */
  char download_file_path[MAX_FILENAME];
  sprintf(download_file_path , "%s/%s", p_download_dir, p_filename);

  /* check if path exists. if not: abort */
  struct stat st = {0};
  if (stat(p_download_dir, &st) == -1) {
    log_record("Path '%s' does not exist. Exiting\n", p_download_dir);
    fprintf(stderr, "Error (%d): %s\n", errno, strerror(errno));
    exit(1);
    }

  /* create file using fseek */
  if (stat(download_file_path, &st) == -1) {
    log_record("File '%s' does not exist. Creating file.\n", 
      download_file_path);
    FILE *file = fopen(download_file_path, "w");
    if (file == NULL) {
      log_record("File '%s' creation error. Exiting\n", download_file_path);
      fprintf(stderr, "Error (%d): %s\n", errno, strerror(errno));
    }
    fseek(file, filesize - 1, SEEK_SET);
    fputc('\0', file);
    fclose(file);
    log_record("File '%s' created!\n", download_file_path);
  }

  /* initial file checksum */
  log_record("Getting chunk_states for final integrity check...\n");
  get_chunk_states(request_info, download_file_path);
  log_record("Chunk states received.\n");
  for (int i=0; i<chunk_total; i++) {
    if (request_info->chunk_states[i] == 0) {
      log_record("Chunk(s) missing. Will attempt to request from peers.\n");
      break;
    }
    else if (i == chunk_total - 1) {
      sha256sum(download_file_path, sha256sum_file);
      if (validate_sha256sum(request_info->info_dict->sha256sum, 
        sha256sum_file) == 0) {
        log_record("Complete file already available.\n");
        log_record("File %s available in directory %s.\n", p_filename, 
          p_download_dir);
        free(seeders);
        return 0;
      }
      /* TODO handle this case when the file checksum doesn't match! */
      log_record("Received file and requested file do not match! Something"
        " malicious is going on...\n");
      free(seeders);
      return 0;
    }
  } 

  /* request & receive seeder info */
  log_record("Not all chunks are present!\n");
  log_record("Requesting seeder information...\n");
  for (int i = 0; i < num_peers; i++) {
    seeders[i].request_info = request_info;
    // printf("DEBUG: Checking seeder at %s\n", request_info->peer_set[i]);
    seeders[i].ip_addr = request_info->peer_set[i];
    ret = pthread_create(&threads[i], NULL, populate_seeder_info, &seeders[i]);
    if (ret) {
      fprintf(stderr, "ERROR: pthread_create() failed.");
      exit(1); 
    }
  }
  log_record("Seeder information obtained from (%d) peers.\n", num_peers);
  for (int i = 0; i< num_peers; i++) {
    pthread_join(threads[i], 0);
  }
  
  /* distribute the chunk requests among connected peers */ 
  for (int i = 0; i<num_peers; i++) {
    // printf("Seeder %d provides chunkset [", i);
    for (int j = 0; j < chunk_total; j++) {
      // printf("%d, ", seeders[i].piece_states[j]);
      if (seeders[i].piece_states[j] == 1 && p_chunk_states[j] == 0 
        && (j % num_peers == i)) {
        // printf("%d, ", seeders[i].piece_states[j]);
        p_chunk_states[j] = 1;
      }
      else if (seeders[i].piece_states[j] == 1) {
        seeders[i].piece_states[j] = 2;
      }
    }
    // printf("]\n");
  }

  /* gaurentee we always have a chunk provider if it is available */
  for (int i = 0; i<num_peers; i++) {
    for (int j = 0; j < chunk_total; j++) {
      if (seeders[i].piece_states[j] == 2 && p_chunk_states[j] == 0) {
        p_chunk_states[j] = 1;
        seeders[i].piece_states[j] = 1;
      }
      else if (seeders[i].piece_states[j] != 1) {
        seeders[i].piece_states[j] = 0;
      }
    }
  }

  /* download all of the chunks into our file from our connected peers*/
  log_record("Downloading chunks from peers...\n");
  printf("Downloading file from (%d) peers.\n", num_peers);
  for (int i = 0; i < num_peers; i++) {
    ret = pthread_create(&threads[i], NULL, download_chunkset_from_peer, 
      &seeders[i]);
    if (ret) {
      perror("ERROR: pthread_create() failed.");
      exit(1); 
    }
  }
  for (int i=0; i < num_peers; i++) {
    pthread_join(threads[i], 0);
    close(seeders[i].sockfd);
    free(seeders[i].piece_states);
  }

  /* final file integrity checksum */
  log_record("Getting chunk_states for final integrity check...\n");
  get_chunk_states(request_info, download_file_path);
  log_record("Chunk states received.\n");
  for (int i=0; i<chunk_total; i++) {
    if (p_chunk_states[i] == 0) {
      log_record("Piece missing. Attempting to re-request peers...\n");
      break;
    }
    else if (i == chunk_total - 1) {
      sha256sum(download_file_path, sha256sum_file);
      if (validate_sha256sum(request_info->info_dict->sha256sum, 
        sha256sum_file) == 0) {
        log_record("Correct file received. Download successful.\n");
        free(seeders);
        return 0;
      }
      /* TODO handle this case when the file checksum doesn't match! */
      log_record("Received file and requested file do not match!\n");
    }
  } 

  free(seeders);
  return 1;
}

void *populate_seeder_info(void* args) 
{
  struct SeederInfo *seeder = (struct SeederInfo*) args;
  seeder->piece_states = malloc(sizeof(int) * 
    seeder->request_info->info_dict->chunk_total);
  for (int i = 0; i < seeder->request_info->info_dict->chunk_total; i++) {
    seeder->piece_states[i] = 0;
  }

  init_connection(P2P_PORTNUM, &seeder->sockfd, seeder->ip_addr);
  tracker_handshake(seeder->sockfd);

  recv(seeder->sockfd, seeder->piece_states, sizeof(int) * 
    seeder->request_info->info_dict->chunk_total, MSG_WAITALL);

  return NULL;
}

void *download_chunkset_from_peer(void* args) 
{
  struct SeederInfo *seeder = (struct SeederInfo*) args;
  struct UsageInfo *request_info = (struct UsageInfo*) seeder->request_info;
  char* buf = malloc(sizeof(char) * BUFSIZ);
  long int file_size;
  ssize_t len;
  int chunk_id;

  int chunk_size = request_info->info_dict->chunk_size;

  /* create the download path */
  char *p_download_dir = request_info->download_dir;
  char *p_filename = request_info->info_dict->file_name;
  char download_file_path[MAX_FILENAME];
  sprintf(download_file_path , "%s/%s", p_download_dir, p_filename);

  // Send the list of things we want from the peer
  send(seeder->sockfd, seeder->piece_states, sizeof(int) * 
    request_info->info_dict->chunk_total, MSG_NOSIGNAL);

  printf("Downloading the chunkset [");
  for (int i = 0; i < request_info->info_dict->chunk_total-1; i++) {
    printf("%d, ", seeder->piece_states[i]);
  }
  printf("%d] from peer %s\n", seeder->piece_states[request_info->info_dict->
    chunk_total-1], seeder->ip_addr);

  /* open our file for writing into */
  FILE* file = fopen(download_file_path, "rb+");
  if (file == NULL) {
    fprintf(stderr, "Could not open file for download. %s", strerror(errno));
    return NULL; // exit and destory objects
  }

  for (int i = 0; i < request_info->info_dict->chunk_total; i++) {

    if (seeder->piece_states[i] != 1) {
      // printf("DEBUG: I do not want chunk id %d!\n", i);
      continue;
    }

    recv(seeder->sockfd, &chunk_id, sizeof(int), MSG_WAITALL);
    // printf("Iteration %d: I am about to recieve chunk_id %d\n", i, 
    //  chunk_id);
    // printf("DEBUG: I want chunk id %d!\n", i);
    recv(seeder->sockfd, &file_size, sizeof(long int), MSG_WAITALL);
    // printf("DEBUG: chunk_size: '%ld'.\n", file_size);

    long int remain_data = file_size;
    if (chunk_id == 0) {
      fseek(file, 0, SEEK_SET);
      // printf("Setting seek to 0\n");
    }
    else {
      fseek(file, (chunk_size*chunk_id), SEEK_SET);
      // printf("Setting seek to %d\n", (chunk_size*chunk_id));
    }
    int total_received_bytes;
    while (remain_data > 0)
    {
      if (remain_data < BUFSIZ) { // we want our buffer size of data
        len = recv(seeder->sockfd, buf, remain_data, MSG_WAITALL);
      }
      else { // we want to recieve only the remaining data
        len = recv(seeder->sockfd, buf, BUFSIZ, MSG_WAITALL);
      }
      if (len < 0) {
        fprintf(stderr, "Error (%d): %s\n", errno, strerror(errno));
        exit(EXIT_FAILURE);
      }
      fwrite(buf, sizeof(char), len, file);
      remain_data -= len;
      total_received_bytes += len;
      //fprintf(stdout, "Receive %ld bytes and we hope :- %ld bytes\n", 
      //  len, remain_data);
    }  
    // printf("I received '%d' many bytes!\n", total_received_bytes);
  }
  fclose(file);
  return NULL;
}

void generate_file(char *filePath) 
{
    // Get name of file from filePath
    char *file = strrchr(filePath, '/');
    if (file == NULL) {
        file = filePath;
    } else {
        file++;
    }

    // Create TORRENTFILE
    char torrentFile[MAX_PATH_LENGTH];
    sprintf(torrentFile, "./%.*s.sly", (int) (strrchr(file, '.') - file), 
      file);

    // Open TORRENTFILE for writing
    int torrentFd = open(torrentFile, O_WRONLY | O_CREAT, 0644);
    if (torrentFd == -1) {
        perror("Error opening TORRENTFILE");
        exit(EXIT_FAILURE);
    }

    // Get file size
    struct stat st;
    if (stat(filePath, &st) == -1) {
        perror("Error getting file size");
        close(torrentFd);
        exit(EXIT_FAILURE);
    }
    off_t fileSize = st.st_size;

    // Calculate number of chunks
    int numPieces = fileSize / PIECE_LENGTH;

    // Write data to TORRENTFILE
    dprintf(torrentFd, "%s\n", file);           // 1. name of the file
    dprintf(torrentFd, "%ld\n", fileSize);      // 2. size in bytes of file

    char sha256hash[65];
    sha256sum(filePath, sha256hash);
    dprintf(torrentFd, "%s\n", sha256hash);     // 3. sha256sum of the file
    printf("Adding sha256sum to file...\n");

    dprintf(torrentFd, "%d\n", PIECE_LENGTH);   // 4. # of bytes in each piece
    dprintf(torrentFd, "%d\n", numPieces);      // 5. # of chunks for file

    close(torrentFd);
}

void request_file(struct UsageInfo *request_info)
{
  tsize_t send_tag, recv_tag;
  char *p_tracker_ip = request_info->info_dict->tracker_ip;
  char *p_filename = request_info->info_dict->file_name;

  send_tag =  FILE_REQUEST;
  send(request_info->sockfd, &send_tag, sizeof(tsize_t), MSG_NOSIGNAL);
  recv(request_info->sockfd, &recv_tag, 1, 0);
  if (recv_tag != REQUEST_APPROVED) {
    printf("ERROR: Unknown exception has occured. Exiting.\n");
    exit(EXIT_FAILURE); 
  }
  printf("File request accepted by tracker (%s).\n", p_tracker_ip);
  send(request_info->sockfd, &(request_info->info_dict->sha256sum), 
    sizeof(char)*65, MSG_NOSIGNAL);
  printf("Searching for file '%s'...\n", p_filename);
  recv(request_info->sockfd, &recv_tag, 1, 0);
  if (recv_tag == REQUEST_NOT_FOUND) {
    printf("File was not available on the network. Cannot fetch peers."
      " Exiting.\n");
    exit(EXIT_SUCCESS); 
  } 
  if (recv_tag != REQUEST_FOUND) {
    printf("ERROR: Unknown exception has occured. Exiting.\n");
    exit(EXIT_FAILURE); 
  }
  printf("Your request '%s' is available on the torrent network," 
    " fetching peers.\n", p_filename);

  recv(request_info->sockfd, &request_info->num_peers, sizeof(int), 0);
  request_info->peer_set = malloc(sizeof(char*)*request_info->num_peers);
  for (int i = 0; i < request_info->num_peers; i++) {
    request_info->peer_set[i] = malloc(INET_ADDRSTRLEN);
  }
  printf("Found (%d) peers seeding on the network.\n", 
    request_info->num_peers);
  for (int i = 0; i < request_info->num_peers; i++) {
    recv(request_info->sockfd, request_info->peer_set[i], 
      INET_ADDRSTRLEN, 0);
    printf("\tPeer %d: (%s)\n", i+1, request_info->peer_set[i]);
  }
}

void seed_file(struct UsageInfo *seed_info)
{
  tsize_t send_tag, recv_tag;
  char *p_tracker_ip = seed_info->info_dict->tracker_ip;
  char *p_filename = seed_info->info_dict->tracker_ip;

  send_tag =  SEED_REQUEST;
  // (1) make seed request to tracker server
  while (recv_tag != SEED_APPROVED) {
    send(seed_info->sockfd, &send_tag, sizeof(tsize_t), MSG_NOSIGNAL);
    recv(seed_info->sockfd, &recv_tag, 1, 0);
    if (recv_tag != SEED_APPROVED) {
      printf("Tracker failed to accept file seed request."
        " Retrying in 10 seconds. \n");
      sleep(10); }
  }
  // (2) send to tracker server the hash of file to seed (and now provide)
  send(seed_info->sockfd, &(seed_info->info_dict->sha256sum), sizeof(char)*65, 
    MSG_NOSIGNAL);
  recv(seed_info->sockfd, &recv_tag, 1, 0);
  if (recv_tag != SEED_SUCCESS) {
    perror("ERROR: Failed to seed file. Maybe the file doesn't exist?\n");
    exit(1); }
  printf("Seed request accepted by tracker (%s).\n", p_tracker_ip);
  printf("You are currently seeding file: '%s' on the torrent network.\n", 
    p_filename);
  printf("Awaiting leechers...\n");
}

void add_file(struct UsageInfo *add_info) 
{
  tsize_t send_tag, recv_tag;
  char *p_tracker_ip = add_info->info_dict->tracker_ip;
  char *p_filename = add_info->info_dict->file_name;

  send_tag = ADD_REQUEST;
  // (1) make add request to tracker server
  while (recv_tag != ADD_APPROVED) {
    send(add_info->sockfd, &send_tag, sizeof(tsize_t), MSG_NOSIGNAL);
    recv(add_info->sockfd, &recv_tag, 1, 0);
    if (recv_tag != ADD_APPROVED) {
      printf("Tracker failed to accept file add request."
        " Retrying in 10 seconds. \n");
      sleep(10); }
  }
  // (2) send to tracker server the hash of file to add
  send(add_info->sockfd, &(add_info->info_dict->sha256sum), sizeof(char)*65, 
    MSG_NOSIGNAL);
  recv(add_info->sockfd, &recv_tag, 1, 0);
  if (recv_tag != ADD_SUCCESS) {
    perror("ERROR: Failed to add file to tracker. Maybe it already exists?\n");
    exit(1); }
  printf("Add request accepted by tracker (%s).\n", p_tracker_ip);
  printf("Your file '%s' is now available on the torrent network.\n", 
    p_filename);
}

void init_from_file(struct InfoDictionary *data) 
{
  int ret, i;
  char *sum_pieces, *single_piece, *sha256sum, *file_name, *tracker_ip;
  FILE *infile;

  sha256sum = malloc(sizeof(char) * 65);
  single_piece = malloc(sizeof(char) * 65);
  file_name = malloc(sizeof(char) * 2056);
  tracker_ip = malloc(sizeof(char) * 50);

  if (data->file_path != NULL) {
    infile = fopen(data->file_path, "r");
    if (infile == NULL) {
      fprintf(stderr, "ERROR: File could not be read!\n");
      exit(EXIT_FAILURE); }

    ret = fscanf(infile, "%s", tracker_ip);
    data->tracker_ip = tracker_ip;
    if(ret == 0) {
      fprintf(stderr, "ERROR: Tracker ip could not be found!\n");
      exit(EXIT_FAILURE); }

    ret = fscanf(infile, "%s", file_name);
    data->file_name = file_name;
    if(ret == 0) {
      fprintf(stderr, "ERROR: Name could not be found!\n");
      exit(EXIT_FAILURE); }

    ret = fscanf(infile, "%d", &data->file_size);
    if(ret == 0) {
      fprintf(stderr, "ERROR: Invalid filesize value initilaized!\n");
      exit(EXIT_FAILURE); }

    ret = fscanf(infile, "%s", sha256sum);
    memcpy(data->sha256sum, sha256sum, sizeof(char)*65);
    free(sha256sum);
    if(ret == 0) {
      fprintf(stderr, "ERROR: Invalid filesum initilaized!\n");
      exit(EXIT_FAILURE); }

    ret = fscanf(infile, "%d", &data->chunk_size);
    if(ret == 0 ) {
      fprintf(stderr, "ERROR: Invalid piece length defined!\n");
      exit(EXIT_FAILURE); }

    ret = fscanf(infile, "%d", &data->chunk_total);
    if(ret == 0 ) {
      fprintf(stderr, "ERROR: Invalid piece total defined!\n");
      exit(EXIT_FAILURE); }
      
    sum_pieces = malloc((sizeof(char)*64) * data->chunk_total  
      + sizeof(char)*1);
    i=0, ret=1;
    while((i < data->chunk_total) && (ret == 1)) {
      ret = fscanf(infile, "%s", single_piece);
      memcpy(sum_pieces+(i*64), single_piece, sizeof(char)*64);
      i++;
    }
    data->chunks = sum_pieces;
    free(single_piece);
    fclose(infile);
  }
}

void tracker_handshake(int sockfd) 
{
  tsize_t comm_tag = HANDSHAKE;
  int ret;
  ret = send(sockfd, &comm_tag, sizeof(tsize_t), MSG_NOSIGNAL);
  if(ret == -1) { 
    perror("ERROR: client-server connection lost.\n");
    exit(EXIT_FAILURE); 
  }
  ret = recv(sockfd, &comm_tag, 1, 0);
  if(ret == -1) { 
    perror("ERROR: client-server connection lost.\n");
    exit(EXIT_FAILURE); 
  }
  if (comm_tag != HANDSHAKE_OK) {
    perror("ERROR: Failed to make client-server handshake."
        " Maybe the server is full?\n");
    exit(EXIT_FAILURE); 
  }
}


void init_connection(int portnum, int *sockfd, char *ip_addr)
{
  struct sockaddr_in saddr;  // server's IP & port number

  *sockfd = socket(PF_INET, SOCK_STREAM, 0);  // create a TCP socket 
  if(*sockfd == -1) { 
    perror("ERROR: socket creation failed.\n"); 
    exit(1); 
  }
  // Initialize the saddr struct that you will pass to connect 
  // server's IP is in argv[1] (a string like "130.58.68.129")
  // inet_pton converts IP from dotted decimal formation 
  // to 32-bit binary address (inet_pton coverts the other way)
  saddr.sin_port =  htons(portnum);
  saddr.sin_family =  AF_INET;
  if(!inet_aton(ip_addr, &(saddr.sin_addr)) ) { 
    perror("inet_aton"); 
    exit(1); 
  }
  if(connect(*sockfd, (struct sockaddr*)&saddr, sizeof(saddr)) == -1) { 
    switch (portnum) {
      case P2T_PORTNUM:
          log_record("FATAL: Attempt to connect to tracker failed. Abort.\n");
          fprintf(stderr, "ERROR: Attempt to connect to tracker failed."
            " Exiting.\n");
      case P2P_PORTNUM:
          log_record("ERROR: Attempt to connect to peer failed."
            " Exiting thread.\n");
      exit(EXIT_FAILURE); 
    }
  }
}

static void parse_args(int ac, char *av[], struct ArgsInfo *args, 
  struct InfoDictionary *info_dict)
{
  int c, usage_mode;
  char *torrent_path, *upload_path, *download_dir, *generate_path;

  torrent_path = NULL;            // path to torrent file (.sly) (-a/-s/-r)
  usage_mode = USAGE_NULL;        // delcares usage of our cli (see usages)
  upload_path = NULL;             // path to directory of file to seed (-s)
  download_dir = NULL;            // path to directory to download file (-r)
  generate_path = NULL;           // path of torrent file to be generated (-g)

  while (1)
  {
    c = getopt(ac, av, "has:r:g:f:");
    if (c == -1)
    { break; } // no more args to parse!
    switch (c)
    {
    case 'a':
        usage_mode = USAGE_ADD;
        break;
    case 's':
        usage_mode = USAGE_SEED;
        upload_path = optarg;
        break;
    case 'r':
        usage_mode = USAGE_REQUEST;
        download_dir = optarg;
        break;  
    case 'g':
        usage_mode = USAGE_GENERATE;
        generate_path = optarg;
      break;
    case 'f':
        torrent_path = optarg;
        break;
    case ':':
        fprintf(stderr, "\n Error -%c missing arg\n", optopt);
        usage();
        exit(EXIT_FAILURE);
    case '?':
        exit(EXIT_FAILURE);
    case 'h':
        usage();
        exit(EXIT_SUCCESS);
    default:
        abort();
    }
  }

  if (usage_mode == USAGE_NULL) {
    fprintf(stderr, "ERROR: No usage defined (-a | -r | -g)\n");
    usage();
    exit(EXIT_FAILURE);

  } if (usage_mode == USAGE_SEED && upload_path == NULL) {
    fprintf(stderr, "ERROR: No file provided -s <file_to_seed>\n");
    usage();
    exit(EXIT_FAILURE);

  } if (usage_mode == USAGE_REQUEST && download_dir == NULL) {
    fprintf(stderr, "ERROR: No download directory specified -r"
      " <download_directory>\n");
    usage();
    exit(EXIT_FAILURE);

  } if (usage_mode == USAGE_GENERATE && generate_path == NULL) {
    fprintf(stderr, "ERROR: No file -g <file_to_generate_torrent>\n");
    usage();
    exit(EXIT_FAILURE);

  } if (torrent_path == NULL) {
    fprintf(stderr, "ERROR: No file defined -f <file_name>\n");
    usage();
    exit(EXIT_FAILURE);
  }
  
  /* intialize datafields of ArgsInfo struct */
  args->info_dict = info_dict;
  args->info_dict->file_path = torrent_path;
  args->usage_mode = usage_mode;
  args->upload_path = upload_path;
  args->download_dir = download_dir;
  args->generate_path = generate_path;
}

static void usage(void)
{
  fprintf(stderr,
          "./peer {(-a | -s <seed_file> | -r <request_dir> | -g <gen_dir>)"
            " -f <sly_file>\n"
          "\t-a add new torrent to the tracker server\n"
          "\t-s seed an existing file on the torrent network\n"
          "\t-r request a file from peers on the torrent network\n"
          "\t-g generate a torrent from file\n"
          "\t-f file_name read in configuration info from a file\n"
          "\t-h print out this message\n");
  exit(-1);
}
