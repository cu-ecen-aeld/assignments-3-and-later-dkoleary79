#!/bin/sh

status=0

#first validate the args
if [ $# -ne 2 ]
then
   echo "!ERR: Expected two arguments"
   status=1
else
   dirName=$1
   searchStr=$2
   if ! [ -d $dirName ]
   then
      echo "!ERR: $dirName is NOT a directory"
      status=1
   fi
fi

#then perform the action
if [ $status -eq 1 ]
then
    echo "Syntax: $0 <filesdir> <searchstr>"
else
    echo "The number of files are `find $dirName -type f | wc -l` and the number of matching lines are `grep -r $searchStr $dirName | wc -l`"
fi

exit $status
