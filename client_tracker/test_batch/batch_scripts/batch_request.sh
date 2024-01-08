#!/bin/bash

# a script that accepts -h -a <argument> -b
while getopts "hf:n:i:s:d:" OPTION
do
   case $OPTION in
       h)
         # if -h, print help function and exit
         helpFunction
         exit 0
         ;;
       f)
         # -f (file)
         FILE=$OPTARG
         ;;
       n)
         # -n (number of peers)
         NUM_PEERS=$OPTARG
         ;;
       i)
         # -i (number of iterations)
         NUM_ITERATIONS=$OPTARG
         ;;
       s)
         # -s (sly file)
         SLY_FILE=$OPTARG
         ;;
       d)
         # -d (download dir)
         DOWNLOAD_DIR=$OPTARG
         ;;
       v)
         VERBOSE=true
         ;;
       ?)
         #echo "ERROR: Unknown option!"
         helpFunction
         exit -1
         ;;
     esac
done

main() {

  # Check that $NUM_PEERS is not NULL
  if [ -z "$NUM_PEERS" ] 
  then
    echo "ERROR: No peer number defined!"
    helpFunction
    exit -1
  fi

  # Check that $NUM_ITERATIONS is not NULL
  if [ -z "$NUM_ITERATIONS" ] 
  then
    echo "ERROR: No iteration number defined!"
    helpFunction
    exit -1
  fi

  # Check that $SLY_FILE is not NULL
  if [ -z "$SLY_FILE" ] 
  then
    echo "ERROR: No .sly file defined!"
    helpFunction
    exit -1
  fi

  # Check that $DOWNLOAD_DIR is not NULL
  if [ -z "$DOWNLOAD_DIR" ] 
  then
    echo "ERROR: No download directory defined!"
    helpFunction
    exit -1
  fi
  
  BINARY='bin/client'
  LOCATION='/home/scasada1/cs87/Project-ldouhov1-scasada1-ylala1/source/client_tracker'
  CHUNK_SIZE=$(basename "${SLY_FILE}" | cut -d. -f1)
  COMMAND=""$LOCATION"/"$BINARY" -f "$SLY_FILE" -r "$DOWNLOAD_DIR""
  OUTPUT="results/"${FILE%.*}"/"$CHUNK_SIZE"-"$NUM_PEERS"p.out"

  if [ ! -d "results/"${FILE%.*}"" ]; then
    echo "Directory results/"${FILE%.*}" does not exist! Creating directory..."
    mkdir "results/"${FILE%.*}""
  fi

  for (( i=0; i<${NUM_ITERATIONS}; i++ )); 
  do
    /usr/bin/time --format="%e" $COMMAND 2>> $OUTPUT
    rm -f $DOWNLOAD_DIR/$FILE
  done
  exit 0
}

main

# IFQUERY_OUT=$(/sbin/ifquery eth0)
# IP_ADDR=$(echo "$IFQUERY_OUT" | awk '/address:/ {print $2}')
# echo "Client IP: ($IP_ADDR)"

# IP_ADDR=130.58.68.193

# script_dir="$(dirname "$(readlink -f "$0")")"

# # for (( i=0 ; i<100 ; i++ )); do
# #     time "$script_dir"/../bin/client -f "$script_dir"/test_files/aot/aot128k.sly -r .
# #     rm -f aot.jpg
# # done

# /usr/bin/time --format="%e" $COMMAND

# for (( i=0 ; i<100 ; i++ )); do
#     time "$script_dir"/../bin/client -f "$script_dir"/test_files/RE4/RE42048k.sly -r . >> output.txt
#     #"$script_dir"/../bin/client -f "$script_dir"/test_files/RE4/RE42048k.sly -r . >> output.txt
#     rm -f RE4.iso
# done
