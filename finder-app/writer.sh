#!/bin/sh
# Writer script for assignment 1
# Author: Tim Bailey

NUM_PARAMS=2
writefile=$1
writestr=$2

# Check for correct number of input parameters
if [ $# -ne $NUM_PARAMS ]
then
	echo "Incorrect number of parameters. Should be $NUM_PARAMS, was $#."
	echo "The first argument is a path to a file on the filesystem."
	echo "The second argument is a text string to write to the file."
	echo "Exiting..."
	exit 1
fi

# Check whether directory exists, create if not
dir=$(dirname "$writefile")
mkdir -v -p "$dir"
if [ $? -ne 0 ]
then
	echo "Failed to create directory."
	echo "Exiting..."
	exit 1
fi

# Write the string to file and check for success
echo "$writestr" > "$writefile"
if [ $? -ne 0 ]
then
	echo "Failed to write to file."
	echo "Exiting..."
	exit 1
fi

echo "$writestr written to $writefile."

