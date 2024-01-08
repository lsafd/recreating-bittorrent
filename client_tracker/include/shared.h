/*
 * Swarthmore College, CS 87
 * Copyright (c) 2020 Swarthmore College Computer Science Department,
 * Swarthmore PA, Professor Tia Newhall
 * 
 * SLY: shared.h
 * Authors: Sasha Casada, Yatin Lala, and Leo Douhovnikoff (12-18-2023)
 */

#ifndef _SHARED_H_
#define _SHARED_H_

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

////////////////////////// .SLY PROTOCOL DEFINITIONS //////////////////////////

    /* .SLY PROTOCOL LISTENING PORT(S): */
    #define P2T_PORTNUM	            1890
    #define P2P_PORTNUM	            1889

    /* CLIENT-TO-SERVER COMMINICATION CODES */
    #define QUIT                    100
    #define HANDSHAKE               101
    #define ADD_REQUEST             102
    #define SEED_REQUEST            103
    #define FILE_REQUEST            104

    /* SERVER-TO-CLIENT COMMINICATION CODES */
    #define HANDSHAKE_OK            200
    #define HANDSHAKE_ERROR         201

    #define ADD_APPROVED            210
    #define ADD_DENIED              211
    #define ADD_SUCCESS		    212 
    #define ADD_FAIL   		    213 

    #define REQUEST_APPROVED        220
    #define REQUEST_FAIL            221
    #define REQUEST_FOUND	    222 
    #define REQUEST_NOT_FOUND       223

    #define SEED_APPROVED           230
    #define SEED_DENIED             231
    #define SEED_SUCCESS            232
    #define SEED_FAIL               233

    #define BUFSIZE   	            256
    // max number of connections tracker will accept
    #define CONMAX   	            256 

    /* OTHER .SLY DEFINITIONS AND SEND CODES */
    #define BACKLOG                 12   
    // BACKLOG specifies how much space the OS should reserve
    // for incomming connections that have not yet been accepted by
    // this server.

////////////////////// BITTORRENT PROTOCOL DEFINITIONS ////////////////////////

    /* BITTORRENT PROTOCOL LISTENING PORT(S): */
    #define BITTORRENT_PORTNUM	    1888

    /* BITTORRENT ERROR CODES */
    #define INVALID_REQUEST         100
    #define MISSING_INFO_HASH       101
    #define MISSING_PEER_ID         102
    #define MISSING_PORT            103
    #define INVALID_INFO_HASH       150
    #define INVALID_PEER_ID         151
    #define INVALID_NUMWANT         152
    #define INFO_HASH_NOT_FOUND     200
    #define REQUEST_TIME_ERR        500     
    #define ERROR                   900 

    /* DEFINITIONS */
    #define INFO_HASH_SIZE          20
    #define PEER_ID_SIZE            20

    /* EVENT SPECIFICATIONS */
    #define STARTED                 1
    #define STOPPED                 2
    #define COMPLETED               3

    /* DEFINE FILE MODES */
    #define SINGLE_FILE             1
    #define MULTI_FILE              2

///////////////////////////////////////////////////////////////////////////////

/**   
 * Type to store/send/recve message tags and message sizes
 * use tsize_t to declare variables storing these values:
 *   tsize_t  len, tag;
 * NOTE: can store up to 255 (related to MSGMAX size)
 **/ 
typedef unsigned char tsize_t; 
typedef int socket_t; 

/**
 * Struct that encapsulates all of the information that is to contained within
 * the metadata of the torrent file.
 **/
typedef struct InfoDictionary {
    char *tracker_ip;           // ip of the torrent network hosting tracker
    char *file_path;            // path to our torrent file (.sly) (-a/-s/-r)
    char *file_name;               
    int file_size;              // length of the file in bytes
    char sha256sum[65];         // SHA256 hexadecmial string of filesum
    int chunk_size;             // number of bytes in each piece
    int chunk_total;            // total number of chunks
    char *chunks;               // concat all piece 20-byte SHA256 hash values

    int filemode;               // filemode of the file; either SINGLE/MULTI
    struct FileInfo *files;     // MULTI mode file_list struct
} info_dictionary_t;

/* Encapsulates file information for MULTI_FILE mode info dictionary member */
typedef struct InfoDictionaryFileInfo {
    int length;                 // length of the file in bytes
    char sha256sum[65];         // SHA256 hexadecmial string of filesum
    char **path;                // list of strings containing path
                                //      i.e, dir1/dir2/file.ext
                                //      --> [d/i/r/1], [d/i/r/2], etc.
} id_file_info_t;

/* Data structure used to recieve list of peers hosting a file */
typedef struct PeerList {
    int sockfd;
    int num_peers;
    struct peer_info* peer_list; 
} peer_list_t;

/* Struct definition which encapsulates data needed for client usage functions */
typedef struct UsageInfo {
    char *upload_path;              // (needed for: USAGE_SEED)
    char *download_dir;             // (needed for: USAGE_REQUEST)
    char *generate_path;            // (needed for: USAGE_GENERATE)

    int num_peers;                  // (needed for: USAGE_REQUEST)
    char** peer_set;                // (needed for: USAGE_SEED)

    int sockfd;
    int *chunk_states;
    struct InfoDictionary* info_dict; 
} usage_info_t;

/* Struct definition for holding command line arguments information */
typedef struct ArgsInfo {
    int usage_mode;                     // delcares the usage of our cli (see usages)
    char *upload_path;                  // path to to the file to seed (-s)
    char *download_dir;                 // path to directory to download file (-r)
    char *generate_path;                // path of a torrent file to be generated (-g)
    struct InfoDictionary *info_dict; 
} args_info_t;

/* Struct definition for encapsulating seeder metadata information */
typedef struct SeederInfo {
    char* ip_addr;
    int sockfd;
    struct UsageInfo *request_info;
    int* piece_states;
    char* pieces_path;
} seeder_info_t; // ?

/* Encapsulates metadata needed for log implementation */
typedef struct log_info {
    FILE * log_file;
    struct timeval start_tv, cur_tv;
} log_info_t;

/**
 * @brief Records a log entry with a timestamp in milliseconds
 * @param format A printf-style format string for the log message
 * @param ... Additional arguments corresponding to the format string
 * @return None
 *
 * @note The log entry includes a timestamp in milliseconds relative to the start
 *       time of the logging session. The log entry is written to the file specified
 *       by the logger structure.
 **/
void log_record(const char * format, ... );

/**
 * @brief Establish and configure a TCP server socket for incoming connections
 * @param portnum The port number to bind the server socket to
 * @param listenfd Pointer to an integer where the server socket file descriptor will be stored
 * @param caddr Pointer to a sockaddr_in structure to store client connection information
 * @param socklen Pointer to an unsigned integer to store the length of the client connection structure
 * @return None
 *
 * @note The BACKLOG constant determines the size of the queue for incoming connections
 *       that have not yet been accepted by the server.
 **/
void host_connection(int portnum, int *listenfd, struct sockaddr_in *caddr, unsigned int *socklen);

/**
 * @brief Retrieves the hash states of file chunks and updates the chunk_states array
 * @param info Pointer to a UsageInfo structure containing relevant information
 * @param file_path The path to the file for which chunk states are to be retrieved
 * @return None
 *
 * @note This function iterates over the file's chunks, computes the SHA256 hash of each chunk,
 *       and compares it with the expected hash stored in the InfoDictionary structure.
 *       The result is stored in the chunk_states array of the UsageInfo structure.
 **/
void get_chunk_states(struct UsageInfo *info, char *file_path);

/**
 * @brief Calculate the SHA-256 hash of a file (using shell)
 * @param path_to_file The path to the file for which the hash is to be calculated
 * @param sha256sum_output Pointer to a character array where the resulting hash will be stored
 * @return None
 **/
void sha256sum(char *path_to_file, char *sha256sum_output);

/**
 * @brief Validate the SHA-256 checksum of a file
 * @param path_to_file The path to the file for which the checksum is to be validated
 * @param sha256_checksum The expected SHA-256 checksum to compare against
 * @return 
 * - 0 if the calculated hash matches the expected checksum (validation successful)
 * - Non-zero value if there is a mismatch (validation failed)
 **/
int validate_sha256sum(char *path_to_file, char* sha256_checksum);

/**
 * @brief Print out all of the information contained within an info_dictionary struct
 * @param info_dict The path info_dictionary struct that is to be printed out
 * @return None
 **/
void print_info_dictionary(struct InfoDictionary *info_dict);

#endif
