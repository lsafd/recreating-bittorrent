#!/bin/bash

helpFunction () {
  echo "USAGE: ./generate_torrent.sh -i <tracker_ip> -f <infile>"
  echo -e "\t-i <tracker_ip>: ip address of tracker to host torrent"
  echo -e "\t-f <infile>:     read in info from an input file"
  echo -e "\t-h:              print out this help message"
}

# a script that accepts -h -a <argument> -b
while getopts "hf:i:" OPTION
do
   case $OPTION in
       h)
         # if -h, print help function and exit
         helpFunction
         exit 0
         ;;
       f)
         # -f (input file)
         FILEPATH=$OPTARG
         ;;
       i)
         # -i (ip address)
         IP_ADDRESS=$OPTARG
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
  # Check that $IP_ADDRESS is not NULL
  if [ -z "$IP_ADDRESS" ] 
  then
    echo "ERROR: No tracker ip defined!"
    helpFunction
    exit -1
  fi

  # Check that $FILEPATH is not NULL
  if [ -z "$FILEPATH" ] 
  then
    echo "ERROR: No filepath defined!"
    helpFunction
    exit -1
  fi

  CURRENT_DIR="$(pwd)"
  MUTLIGEN_DIR="$CURRENT_DIR/multigen_scripts"
  FILE="$(basename $FILEPATH)"
  rm -rf ./${FILE%.*}
  OUTPUT_DIR=${FILE%.*}

  # Loop over all generation scripts
  if [ -d "$MUTLIGEN_DIR" ]; then
    for script in "$MUTLIGEN_DIR"/*; do
      echo "Generating .sly..."; 
      ( source $script -i $IP_ADDRESS -f $FILEPATH )
    done
  fi

  FILE="$(basename $FILEPATH)"
  mkdir ./${FILE%.*}
  mv ./*.sly ./$OUTPUT_DIR

  echo "Done."
  exit 0
}

main
