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

  # Get name of file from FILEPATH
  FILE="$(basename $FILEPATH)"

  # Create TORRENTFILE
  TORRENTFILE="./${FILE%.*}.sly"
  PIECE_LENGTH=131072
  PIECE_LENGTHk=128K       

  read SIZE _ < <(du -b --max-depth=0 $FILEPATH)
  NUM_PIECES=$(echo "($SIZE + $PIECE_LENGTH - 1)/$PIECE_LENGTH" | bc)

  echo "$IP_ADDRESS" > $TORRENTFILE     # 1. tracker ip address
  echo "$FILE" >> $TORRENTFILE          # 2. name of the file
  echo "$SIZE" >> $TORRENTFILE          # 3. size in bytes of the file

  echo "Hashing $FILEPATH file..."; 
  read SHA256 _ < <(sha256sum $FILEPATH)
  echo "$SHA256" >> $TORRENTFILE        # 4. sha256sum of the file itself
  echo "Adding sha256sum to file..."

  echo "$PIECE_LENGTH" >> $TORRENTFILE # 5. number of bytes in each piece
  echo "$NUM_PIECES" >> $TORRENTFILE    # 6. number of pieces for file

 
  # # Attempt to create temporary directory ./split/<FILE>
  # TEMPDIR="./split/${FILE%.*}"
  # echo "Create temporary directory: ($TEMPDIR) (Y/N)?"
  # read USER_INPUT
  # if [[ ${USER_INPUT,,} != "yes" ]] && [[ ${USER_INPUT,,} != "y" ]]
  # then
  #   echo "Cannot create directory. Aborting."
  #   exit 0
  # fi 
  # mkdir $TEMPDIR 
  
  # # Split the file into 128K chunks
  # split --bytes $PIECE_LENGTH --numeric-suffixes $FILEPATH $TEMPDIR/

  OFFSET_TAIL=0;
  OFFSET_HEAD=$PIECE_LENGTH;

  # Loop and hash all chuncks to .sly file
  for (( i=0 ; i<NUM_PIECES ; i++ )); do

    # Corret tail offset for non-zero case.
    if [[ i != 0 ]]
    then
      OFFSET_TAIL=$(echo "( ( $PIECE_LENGTH * $i ) + 1 )" | bc)
    fi
    
    # Corret tail offset for non-zero case.
    if [[ i == $(expr $NUM_PIECES - 1) ]]
    then 
      OFFSET_HEAD=$(echo "($SIZE - ( ( $NUM_PIECES - 1) * $PIECE_LENGTH ) )" | bc)
    fi

    echo "Hashing $FILEPATH file..."; 
    read SHA256 _ < <(tail -c +$OFFSET_TAIL $FILEPATH | head -c $OFFSET_HEAD | sha256sum | awk '{ print $1 }')
    echo "$SHA256" >> $TORRENTFILE
    echo "Adding sha256sum to $FILEPATH file..."

  done

  # # Clean up temporary directory
  # echo "Remove temporary directory: ($TEMPDIR) (Y/N)?"
  # read USER_INPUT
  # if [[ ${USER_INPUT,,} != "yes" ]] && [[ ${USER_INPUT,,} != "y" ]]
  # then
  #   echo "Cannot remove directory. Aborting."
  #   exit 0
  # fi 
  # rm -rf $TEMPDIR

  echo "Done."
  exit 0
}

main
