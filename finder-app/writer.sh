#!/bin/sh

# Writer script for assignment 1
# Author: Iyona Lynn Noronha
# Ref:
# How to create bash script: https://www.datacamp.com/tutorial/how-to-write-bash-script-tutorial
# Runtime arguments processing:https://www.baeldung.com/linux/use-command-line-arguments-in-bash-script
# 
# 

#Input Error Conditions Handling
#Both runtime argument are reqd else error
if [ $# -ne 2 ]; then
    echo "Error: Two arguments required"
	echo "./writer.sh <writefile> <writestr>"
    exit 1
fi

# Accept the runtime arguments
writefile=$1
writestr=$2

#Extract file name and overwriting the string prompt given to chatgpt
# Extract the directory path from the file path
dirpath=$(dirname "$writefile")

# Create the directory path if it does not exist
mkdir -p "$dirpath"

# Write the string to the file, overwriting if it already exists
if echo "$writestr" > "$writefile"; then
    echo "Successfully wrote to file: $writefile"
else
    echo "Error: Could not create or write to file '$writefile'"
    exit 1
fi

exit 0
