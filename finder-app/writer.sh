#!/bin/sh

status=0

#first validate the args
if [ $# -ne 2 ]
then
   echo "!ERR: Expected two arguments"
   status=1
else
   filePath=$1
   writeStr=$2
fi

#then perform the action
if [ $status -eq 1 ]
then
    echo "Syntax: $0 <filePath> <writeStr>"
else
    mkdir -p "$(dirname "$filePath")" && touch "$filePath"
    if [ $? -ne 0 ]
    then
        echo "!ERR: failed to create file"
        status=1
    else
        echo "$2" > $filePath
    fi
fi

exit $status
