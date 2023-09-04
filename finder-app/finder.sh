#!/bin/sh
# Finder script for assignment 1
# Author: Tim Bailey

NUM_PARAMS=2
filesdir="$1"
searchstr="$2"

# Check for correct number of input parameters
if [ $# -ne $NUM_PARAMS ]
then
	echo "Incorrect number of parameters. Should be $NUM_PARAMS, was $#."
	echo "The first argument is a path to a directory on the filesystem."
	echo "The second argument is a text string to search for within those files."
	echo "Exiting..."
	exit 1
fi

# Check if input directory is valid
if [ ! -d "$filesdir" ]
then
	echo "$filesdir is not a valid directory."
	echo "Exiting..."
	exit 1
fi

# Count files and count lines
total_files=0
total_lines=0
for file in $filesdir/*
do
	total_files=$((total_files+1))
	total_lines=$((total_lines + $(grep -c "$searchstr" "$file")))
done

echo "The number of files are $total_files and the number of matching lines are $total_lines"
exit 0

