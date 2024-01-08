#!/bin/bash

# a script that accepts -h -a <argument> -b
while getopts "hn:s:f:" OPTION
do
   case $OPTION in
       h)
         # if -h, print help function and exit
         helpFunction
         exit 0
         ;;
       n)
         # -n (number of peers)
         NUM_PEERS=$OPTARG
         ;;
       s)
         # -s (sly file)
         SLY_FILE=$OPTARG
         ;;
       f)
         # -f (seed directory)
         SEED_DIR=$OPTARG
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

  # Check that $SLY_FILE is not NULL
  if [ -z "$SLY_FILE" ] 
  then
    echo "ERROR: No .sly file defined!"
    helpFunction
    exit -1
  fi

  # Check that $SEED_DIR is not NULL
  if [ -z "$SEED_DIR" ] 
  then
    echo "ERROR: No seed directory defined!"
    helpFunction
    exit -1
  fi

  LOCATION='/home/scasada1/cs87/Project-ldouhov1-scasada1-ylala1/source/client_tracker'
  BINARY='bin/client'
  echo -e "\tMy seed file is: $SEED_FILE"
  echo -e "\tMy .sly file is: $SLY_FILE"
  COMMAND=""$LOCATION"/"$BINARY" -f "$SLY_FILE" -s "$SEED_DIR""

  MACHINES=(
    "mew"
    "abra"
    "beedrill"
    "bulbasaur"
    "butterfree"
    "charmander"
    "clefairy"
    "diglett"
    "ditto"
    "dratini"
    "eevee"
    "ekans"
    "gastly"
    "geodude"
    "horsea"
    "koffing"
    "mankey"
    "meowth"
    "oddish"
    "onix"
    "pidgey"
    "pikachu"
    "poliwag"
    "ponyta"
    "psyduck"
    "rattata"
    "seel"
    "jigglypuff"
    "snorlax"
    "squirtle"
    "staryu"
    "carrot"
    "mustard"
    "mace"
    "cornstarch"
    "poppy"
    "spinach"
    "pepper"
    "bacon"
    "caper"
    "onion"
    "honey"
    "cream"
    "nutmeg"
    "cabbage"
    "coriander"
    "mugwort"
    "cheese"
    "celery"
    "oil"
    "oregano"
    "thyme"
    "rosemary"
    "tomato"
    "flour"
    "lime"
    "sage"
    "turmeric"
    "butter"
    "horseradish"
    "pimento"
    "owl"
    "hawk"
    "pelican"
    "pigeon"
    "puffin"
    "quail"
    "raven"
    "grebe"
    "seagull"
    "sparrow"
    "myrtle"
    "oriole"
    "macaw"
    "magpie"
    "swan"
    "loon"
    "lark"
    "kestrel"
    "ibis"
    "wren"
    "grouse"
    "flamingo"
    "finch"
    "falcon"
    "egret"
    "eagle"
    "dodo"
    "cuckoo"
    "crane"
    "cardinal"
    "heron"
    "buzzard"
    "ostrich"
    "bluejay"
    #"growlithe"
  )

  for (( i=0; i<${NUM_PEERS}; i++ )); 
  do
    ssh scasada1@"${MACHINES[$i]}" $COMMAND & 
  done
  
  exit 0
}

main