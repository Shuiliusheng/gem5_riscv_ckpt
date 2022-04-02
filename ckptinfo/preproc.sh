#!/bin/bash
if [[ $# < 1 ]]; then
    echo "parameter is not enought!"
    exit
fi
cat $1 | awk -F '{' '{print "{"$2}' >temp.log
mv temp.log $1
