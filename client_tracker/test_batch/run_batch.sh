#!/bin/bash

# a script that accepts -h -a <argument> -b
while getopts "hf:" OPTION
do
   case $OPTION in
       h)
         # if -h, print help function and exit
         helpFunction
         exit 0
         ;;
       f)
         # -f (test file)
         TEST_FILE=$OPTARG
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

  # Check that $TEST_FILE is not NULL
  if [ -z "$TEST_FILE" ] 
  then
    echo "ERROR: No test file defined!"
    exit -1
  fi

  # (7) total peer sets
  PEER_SET=(
    "1"
    "2"
    "4"
    "8"
    "16"
    # "32"
    # "64"
  )

  LOCATION='/home/scasada1/cs87/Project-ldouhov1-scasada1-ylala1/source/client_tracker/test_batch'
  BINARY='bin/client'

  COMMAND=""$LOCATION"/"$BINARY" -f "$SLY_FILE" -s "$SEED_FILE""

  BATCH_FILES_DIR="batch_files/${TEST_FILE%.*}"

  TRACKER="batch_scripts/batch_tracker.sh"

  SEED="batch_scripts/batch_seed.sh"
  SEED_DIR="$LOCATION/batch_files/${TEST_FILE%.*}"

  RESET="batch_scripts/batch_reset.sh"

  REQUEST="batch_scripts/batch_request.sh"
  REQUEST_DIR="$LOCATION/.download"


  echo "Checking $BATCH_FILES_DIR exists..."

  # Check if target is a directory
  if [ ! -d "$BATCH_FILES_DIR" ]; then
    echo "Directory $BATCH_FILES_DIR does not exist!"
    exit 1
  fi

  # Loop through files in the target directory for confirmation
  echo "Found the following files:"
  for sly in "$BATCH_FILES_DIR"/*.sly; do
    if [ -f "$sly" ]; then
        echo -e "\t$sly"
    fi
  done
  echo "Initializing batch job over .sly directories..."

  # Begin out-most loop
  for sly in "$BATCH_FILES_DIR"/*.sly; do
    if [ -f "$sly" ]; then
        for peer_num in ${PEER_SET[@]}; do
            timeout 5 $RESET > /dev/null 2>&1
            sleep 5
            fuser -k 1890/tcp # kill tracker
            $TRACKER & > /dev/null 2>&1
            sleep 2
            $SEED -n $peer_num -s $LOCATION/$sly -f $SEED_DIR & 
            sleep 5
            $REQUEST -f $TEST_FILE -n $peer_num -i 5 -s $LOCATION/$sly -d $REQUEST_DIR
            sleep 5
        done
    fi
  done
}

main
