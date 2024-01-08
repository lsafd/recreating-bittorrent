/*
 * SLY: run_batch.sh
 * Bash script suite for testing CS87 Final Project, SLY File-Sharing Implementation.
 *
 * This is released into the public domain.
 * Authors: Sasha Casada, Yatin Lala, and Leo Douhovnikoff (12-05-2023)
 * Written by Sasha Casada <scasada1@swarthmore.edu>.
 */

Here's a brief explanation on how to use our batch testing pipeline for the SLY 
system.

If you have a file that you would like to test on, navigate to `batch_files/` 
and run `multigen_ests.sh -i <tracker_ip_address> -f <path_to_file>`. This 
will automatically generate a directory for that file in `batch_files/` named 
after the file that you have selected. Once you have done this, move your file 
to the directory that has been created for you (NOTE: This would be done 
automatically, however, due to low quota limits on the lab machines, we ask 
that this is done manually). Once this has been complete, you are now able to 
run `run_batch.sh -f <file_name>` where the file name is the name of the file 
that you have moved into the file's respective directory in: 
`batch_files/FILENAME/...`

This will automatically run tests as outlined in `run_batch.sh` where we run 5 
tests for 7 different peer set sizes (from 1 peer to 64 peers), and we will 
repeat this for each of the 5 different sizes of .sly files (where chunks go 
from 128k size to 2048k in size).

Each of the results of these runs will be contained in: 
`results/FILENAME/<chunk_size>-<peer_num>.out`. You can view all of these. We 
also have provided a short script for generating the delta (standard-deviation) 
of each of these runs. 

Of course, for any other details, ask myself (scasada1), or check the source 
code for more clarity!
