#!/bin/sh

# Writer script for assignment 1
# Author: Iyona Lynn Noronha
# Ref:
# How to create bash script: https://www.datacamp.com/tutorial/how-to-write-bash-script-tutorial
# Runtime arguments processing:https://www.baeldung.com/linux/use-command-line-arguments-in-bash-script
# Extract directory path prompt given to chatgpt
# Directory related:https://www.ibm.com/docs/en/aix/7.1?topic=directories-creating-mkdir-command

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

# Extract the directory path from the file path
writedir=$(dirname "$writefile")

# Create directory only if it doesn't exists else error
if [ ! -d "$writedir" ]; then
    mkdir -p "$writedir"
fi

# Write the string to the file, overwriting if it already exists
if echo "$writestr" > "$writefile"; then
    echo "Successfully wrote $writestr to file: $writefile"
else
    echo "Error: Writing to '$writefile' failed"
    exit 1
fi

exit 0
