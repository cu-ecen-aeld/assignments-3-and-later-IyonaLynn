#!/bin/sh

# Finder script for assignment 1
# Author: Iyona Lynn Noronha
# Ref:
# How to create bash script: https://www.datacamp.com/tutorial/how-to-write-bash-script-tutorial
# Runtime arguments processing:https://www.baeldung.com/linux/use-command-line-arguments-in-bash-script
# File count command:https://stackoverflow.com/questions/11307257/is-there-a-bash-command-which-counts-files
# Matching line count command: https://www.cyberciti.biz/faq/grep-count-lines-if-a-string-word-matches/
	   
	   
#Input Error Conditions Handling
#Both runtime argument are reqd else error
if [ $# -ne 2 ]; then
    echo "Error: Two arguments required"
	echo "./finder.sh <filesdir> <searchstr>"
    exit 1
#filesdir does not represent a directory on the filesystem
elif [ ! -d "$1" ]; then
	echo "<filesdir> is not a directory on the filesystem"
	exit 1
fi

# Accept the runtime arguments
filesdir=$1
searchstr=$2

# number of files in the directory and all subdirectories
file_count=$(find "$filesdir" -type f | wc -l)

# number of matching lines in respective files, [line which contains searchstr]
line_count=$(grep -r "$searchstr" "$filesdir" | wc -l)

# Print the result
echo "The number of files are $file_count and the number of matching lines are $line_count"

exit 0
